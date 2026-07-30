#include "lighting.hpp"
#include "../handlers/textures.hpp"
#include "../item/texture.hpp"
#include "../config/config.hpp"
#include "../gui/console.hpp"
#include "../utils/logger.hpp"
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

float OkLighting::_tint[3]     = {1.0f, 1.0f, 1.0f};
float OkLighting::_fogColor[3] = {0.75f, 0.80f, 0.85f};
float OkLighting::_fogDensity  = 0.0f;
float OkLighting::_sunColor[3] = {1.0f, 1.0f, 1.0f};
float OkLighting::_sunDir[3]   = {0.0f, -1.0f, 0.0f};
float OkLighting::_zenith[3]   = {0.25f, 0.48f, 0.80f};
float OkLighting::_ambient     = 0.55f;

float OkLighting::_lightPos[OkLighting::MAX_LIGHTS][3];
float OkLighting::_lightColor[OkLighting::MAX_LIGHTS][3];
float OkLighting::_lightRadius[OkLighting::MAX_LIGHTS];
int   OkLighting::_lightCount      = 0;
long  OkLighting::_lightGeneration = 0;

// The atmosphere curve: one keyframe per anchor hour, linearly interpolated
// around the clock (the last segment wraps 23 -> 5). The palette follows the
// agreed look: neutral day, warm sunset, cold teal night.
// NOLINTBEGIN(readability-magic-numbers)
struct OkAtmoKey {
  float hour;
  float tint[3];      // scene tint (multiplies albedo)
  float fog[3];       // fog colour (also the sky at the horizon)
  float density;      // exponential fog density (per metre)
  float sun[3];       // sun colour (consumed by the directional stage later)
  float zenith[3];    // sky colour straight up (skybox gradient top)
  float ambient;      // flat ambient floor under the Gouraud sun
};

static const OkAtmoKey ATMO_KEYS[] = {
    // deep night: cold teal soak, dense milky haze (no sun: the flat
    // ambient carries the whole city, the tint does the darkening)
    {0.0f, {0.30f, 0.40f, 0.48f}, {0.10f, 0.16f, 0.20f}, 0.0060f,
     {0.0f, 0.0f, 0.0f}, {0.02f, 0.05f, 0.09f}, 0.78f},
    {5.0f, {0.30f, 0.40f, 0.48f}, {0.10f, 0.16f, 0.20f}, 0.0060f,
     {0.0f, 0.0f, 0.0f}, {0.02f, 0.05f, 0.09f}, 0.78f},
    // dawn: haze warms and thins
    {7.0f, {0.85f, 0.75f, 0.70f}, {0.70f, 0.60f, 0.55f}, 0.0035f,
     {1.0f, 0.75f, 0.55f}, {0.30f, 0.35f, 0.50f}, 0.65f},
    // day: neutral light, light blue distance haze
    {9.0f, {1.00f, 1.00f, 1.00f}, {0.72f, 0.78f, 0.85f}, 0.0018f,
     {1.0f, 0.98f, 0.92f}, {0.25f, 0.48f, 0.80f}, 0.55f},
    {17.0f, {1.00f, 1.00f, 1.00f}, {0.72f, 0.78f, 0.85f}, 0.0018f,
     {1.0f, 0.98f, 0.92f}, {0.25f, 0.48f, 0.80f}, 0.55f},
    // sunset: everything warms, haze turns amber-pink
    {20.0f, {1.00f, 0.72f, 0.52f}, {0.75f, 0.50f, 0.42f}, 0.0032f,
     {1.0f, 0.55f, 0.30f}, {0.25f, 0.22f, 0.38f}, 0.60f},
    // dusk into night: the teal takes over
    {22.0f, {0.42f, 0.48f, 0.55f}, {0.16f, 0.22f, 0.27f}, 0.0050f,
     {0.2f, 0.15f, 0.15f}, {0.04f, 0.08f, 0.14f}, 0.72f},
    {23.0f, {0.30f, 0.40f, 0.48f}, {0.10f, 0.16f, 0.20f}, 0.0060f,
     {0.0f, 0.0f, 0.0f}, {0.02f, 0.05f, 0.09f}, 0.78f},
};
static const int ATMO_KEY_COUNT = (int)(sizeof(ATMO_KEYS) / sizeof(ATMO_KEYS[0]));
// NOLINTEND(readability-magic-numbers)

/**
 * @brief Register the console commands and log the starting state. The
 *        clock itself lives in the config (lighting.time,
 *        lighting.timescale) so it is scriptable like everything else.
 */
void OkLighting::initialize() {
  OkConsole::registerCommand(
      "time", "time [hour]: read or set the day clock (0-24)",
      [](const std::vector<std::string> &args) {
        if (args.empty()) {
          OkConsole::print("time = " +
                           std::to_string(OkLighting::getTimeOfDay()));
          return;
        }
        OkLighting::setTimeOfDay((float)atof(args[0].c_str()));
        OkConsole::print("time = " +
                         std::to_string(OkLighting::getTimeOfDay()));
      });
  OkConsole::registerCommand(
      "timescale",
      "timescale [x]: how much faster than real time the clock runs",
      [](const std::vector<std::string> &args) {
        if (args.empty()) {
          OkConsole::print(
              "timescale = " +
              std::to_string(OkConfig::getFloat("lighting.timescale")));
          return;
        }
        OkConfig::setFloat("lighting.timescale",
                           (float)atof(args[0].c_str()));
        OkConsole::print(
            "timescale = " +
            std::to_string(OkConfig::getFloat("lighting.timescale")));
      });

  update(0.0f);
  OkLogger::info("Lighting",
                 "Day cycle at " + std::to_string(getTimeOfDay()) + "h");
}

/**
 * @brief Current fog density; zero while the lighting.fog toggle is off
 *        (the fog colour is still evaluated -- the skybox horizon and the
 *        clear colour keep following the cycle).
 */
float OkLighting::getFogDensity() {
  return OkConfig::getBool("lighting.fog") ? _fogDensity : 0.0f;
}

float OkLighting::getTimeOfDay() {
  return OkConfig::getFloat("lighting.time");
}

void OkLighting::setTimeOfDay(float hours) {
  float wrapped = std::fmod(hours, 24.0f);
  if (wrapped < 0.0f) {
    wrapped += 24.0f;
  }
  OkConfig::setFloat("lighting.time", wrapped);
}

/**
 * @brief Advance the clock by dt (milliseconds of real time, scaled by
 *        lighting.timescale) and refresh the interpolated values.
 */
void OkLighting::update(float dt) {
  float timescale = OkConfig::getFloat("lighting.timescale");
  if (timescale > 0.0f && dt > 0.0f) {
    float hoursAdvance = (dt / 1000.0f) * timescale / 3600.0f;
    setTimeOfDay(getTimeOfDay() + hoursAdvance);
  }
  evaluate(getTimeOfDay(), _tint, _fogColor, _fogDensity, _sunColor, _sunDir,
           _zenith, &_ambient);
}

/**
 * @brief Interpolate the atmosphere curve at an arbitrary hour. The sun
 *        direction comes from the hour itself (elevation follows a sine
 *        over the 6h..21h daylight arc, azimuth sweeps east to west).
 */
void OkLighting::evaluate(float hours, float outTint[3], float outFogColor[3],
                          float &outFogDensity, float outSunColor[3],
                          float outSunDir[3], float outZenith[3],
                          float *outAmbient) {
  float h = std::fmod(hours, 24.0f);
  if (h < 0.0f) {
    h += 24.0f;
  }

  // Find the surrounding keyframes (wrapping around midnight).
  const OkAtmoKey *prev = &ATMO_KEYS[ATMO_KEY_COUNT - 1];
  const OkAtmoKey *next = &ATMO_KEYS[0];
  float            span = (24.0f - prev->hour) + next->hour;
  float            frac = 0.0f;
  if (h < ATMO_KEYS[0].hour) {
    frac = (h + (24.0f - prev->hour)) / span;
  } else {
    for (int i = 0; i < ATMO_KEY_COUNT; i++) {
      if (ATMO_KEYS[i].hour <= h) {
        prev = &ATMO_KEYS[i];
        next = (i + 1 < ATMO_KEY_COUNT) ? &ATMO_KEYS[i + 1] : &ATMO_KEYS[0];
      }
    }
    if (next == &ATMO_KEYS[0]) {
      span = (24.0f - prev->hour) + next->hour;
      frac = (h - prev->hour) / span;
    } else {
      span = next->hour - prev->hour;
      frac = (span > 0.0f) ? (h - prev->hour) / span : 0.0f;
    }
  }

  for (int c = 0; c < 3; c++) {
    outTint[c]     = prev->tint[c] + (next->tint[c] - prev->tint[c]) * frac;
    outFogColor[c] = prev->fog[c] + (next->fog[c] - prev->fog[c]) * frac;
    outSunColor[c] = prev->sun[c] + (next->sun[c] - prev->sun[c]) * frac;
    if (outZenith != nullptr) {
      outZenith[c] =
          prev->zenith[c] + (next->zenith[c] - prev->zenith[c]) * frac;
    }
  }
  outFogDensity = prev->density + (next->density - prev->density) * frac;
  if (outAmbient != nullptr) {
    *outAmbient = prev->ambient + (next->ambient - prev->ambient) * frac;
  }

  // Sun direction: elevation follows a sine across the 6h..21h daylight
  // arc (below the horizon at night), azimuth sweeps east to west. The
  // vector points FROM the sun TOWARD the scene (ready for lighting).
  // NOLINTBEGIN(readability-magic-numbers)
  float dayFrac  = (h - 6.0f) / 15.0f;  // 0 at 6h, 1 at 21h
  float elev     = std::sin(dayFrac * 3.14159265f) * 1.2f;  // radians, peak ~69 deg
  float azim     = (dayFrac - 0.5f) * 3.14159265f;          // -90..+90 deg
  if (dayFrac < 0.0f || dayFrac > 1.0f) {
    elev = -0.3f;  // parked below the horizon at night
  }
  // NOLINTEND(readability-magic-numbers)
  float cosE = std::cos(elev);
  outSunDir[0] = -std::sin(azim) * cosE;
  outSunDir[1] = -std::sin(elev);
  outSunDir[2] = -std::cos(azim) * cosE;
}


// --- Point lights (L4) -----------------------------------------------

int OkLighting::registerLight(float x, float y, float z, float r, float g,
                              float b, float radius) {
  if (_lightCount >= MAX_LIGHTS) {
    OkLogger::warning("Lighting", "Point light registry full");
    return -1;
  }
  int id             = _lightCount;
  _lightPos[id][0]   = x;
  _lightPos[id][1]   = y;
  _lightPos[id][2]   = z;
  _lightColor[id][0] = r;
  _lightColor[id][1] = g;
  _lightColor[id][2] = b;
  _lightRadius[id]   = radius;
  _lightCount++;
  _lightGeneration++;
  return id;
}

void OkLighting::clearLights() {
  _lightCount = 0;
  _lightGeneration++;
}

long OkLighting::getLightGeneration() { return _lightGeneration; }

int OkLighting::getLightCount() { return _lightCount; }

/**
 * @brief The most relevant lights for a point: sorted by distance
 *        normalized by radius, so a big far light can beat a small
 *        nearer one. Linear scan -- the registry is small and items
 *        cache the result until the registry changes.
 */
int OkLighting::getNearestLights(float x, float y, float z, int *outIdx,
                                 int maxN) {
  float bestScore[8];
  int   n = 0;
  if (maxN > 8) {
    maxN = 8;
  }
  for (int i = 0; i < _lightCount; i++) {
    float dx = _lightPos[i][0] - x;
    float dy = _lightPos[i][1] - y;
    float dz = _lightPos[i][2] - z;
    float d  = std::sqrt(dx * dx + dy * dy + dz * dz);
    float s  = d / (_lightRadius[i] > 1.0f ? _lightRadius[i] : 1.0f);
    // insertion into the small sorted best list
    int   at = n;
    for (int j = 0; j < n; j++) {
      if (s < bestScore[j]) {
        at = j;
        break;
      }
    }
    if (at < maxN) {
      int last = (n < maxN) ? n : maxN - 1;
      for (int j = last; j > at; j--) {
        bestScore[j] = bestScore[j - 1];
        outIdx[j]    = outIdx[j - 1];
      }
      bestScore[at] = s;
      outIdx[at]    = i;
      if (n < maxN) {
        n++;
      }
    }
  }
  return n;
}

const float *OkLighting::getLightPosition(int idx) { return _lightPos[idx]; }

const float *OkLighting::getLightColor(int idx) { return _lightColor[idx]; }

float OkLighting::getLightRadius(int idx) { return _lightRadius[idx]; }

/**
 * @brief Shared radial halo texture: a soft white disc whose alpha
 *        falls off quadratically to the edge. Tinted per light and
 *        drawn additively by halo billboards.
 */
OkTexture *OkLighting::getHaloTexture() {
  OkTexture *existing = OkTextureHandler::getInstance()->getTexture("ok_halo");
  if (existing != nullptr) {
    return existing;
  }
  const int     SIZE = 64;
  unsigned char rgba[SIZE * SIZE * 4];
  for (int y = 0; y < SIZE; y++) {
    for (int x = 0; x < SIZE; x++) {
      float dx = (x + 0.5f) / SIZE - 0.5f;
      float dy = (y + 0.5f) / SIZE - 0.5f;
      float d  = std::sqrt(dx * dx + dy * dy) * 2.0f;  // 0 centre, 1 edge
      float a  = 1.0f - d;
      if (a < 0.0f) {
        a = 0.0f;
      }
      a               = a * a;  // quadratic falloff, soft rim
      int off         = (y * SIZE + x) * 4;
      rgba[off]       = 255;
      rgba[off + 1]   = 255;
      rgba[off + 2]   = 255;
      rgba[off + 3]   = (unsigned char)(a * 255.0f);
    }
  }
  return OkTextureHandler::getInstance()->createTextureFromRawData(
      "ok_halo", rgba, SIZE, SIZE, 4);
}
