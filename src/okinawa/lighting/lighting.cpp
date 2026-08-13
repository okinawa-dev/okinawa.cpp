#include "lighting.hpp"
#include "../config/config.hpp"
#include "../gui/console.hpp"
#include "../handlers/textures.hpp"
#include "../item/texture.hpp"
#include "../utils/logger.hpp"
#include <algorithm>
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
float OkLighting::_lightDir[OkLighting::MAX_LIGHTS][3];
float OkLighting::_lightCosCone[OkLighting::MAX_LIGHTS];
float OkLighting::_lightIntensity[OkLighting::MAX_LIGHTS];
float OkLighting::_pointLightLevel = 0.0f;
int   OkLighting::_lightCount      = 0;
long  OkLighting::_lightGeneration = 0;

// The atmosphere curve: one keyframe per anchor hour, linearly interpolated
// around the clock (the last segment wraps 23 -> 5). The palette follows the
// agreed look: neutral day, warm sunset, cold teal night.
// NOLINTBEGIN(readability-magic-numbers)
// The engine's DEFAULT curve: a neutral clear day and a plain blue
// night. It exists so any project renders sensibly out of the box; a
// game with its own look replaces it through setAtmosphereCurve, which
// is where artistic direction belongs.
// NOLINTBEGIN(readability-magic-numbers)
static const OkAtmosphereKey ATMO_DEFAULT[] = {
    // night
    {0.0f,
     {0.34f, 0.38f, 0.46f},
     {0.08f, 0.10f, 0.15f},
     0.0040f,
     {0.0f, 0.0f, 0.0f},
     {0.03f, 0.05f, 0.10f},
     0.30f},
    {5.0f,
     {0.34f, 0.38f, 0.46f},
     {0.08f, 0.10f, 0.15f},
     0.0040f,
     {0.0f, 0.0f, 0.0f},
     {0.03f, 0.05f, 0.10f},
     0.30f},
    // dawn
    {7.0f,
     {0.88f, 0.84f, 0.80f},
     {0.66f, 0.63f, 0.62f},
     0.0028f,
     {1.0f, 0.86f, 0.72f},
     {0.32f, 0.42f, 0.60f},
     0.50f},
    // day
    {9.0f,
     {1.00f, 1.00f, 1.00f},
     {0.72f, 0.80f, 0.90f},
     0.0012f,
     {1.0f, 0.98f, 0.94f},
     {0.26f, 0.50f, 0.85f},
     0.55f},
    {17.0f,
     {1.00f, 1.00f, 1.00f},
     {0.72f, 0.80f, 0.90f},
     0.0012f,
     {1.0f, 0.98f, 0.94f},
     {0.26f, 0.50f, 0.85f},
     0.55f},
    // sunset
    {20.0f,
     {1.00f, 0.82f, 0.68f},
     {0.74f, 0.58f, 0.50f},
     0.0026f,
     {1.0f, 0.70f, 0.45f},
     {0.28f, 0.30f, 0.48f},
     0.50f},
    // dusk
    {22.0f,
     {0.46f, 0.50f, 0.58f},
     {0.16f, 0.20f, 0.28f},
     0.0036f,
     {0.2f, 0.18f, 0.20f},
     {0.06f, 0.10f, 0.18f},
     0.38f},
    {23.0f,
     {0.34f, 0.38f, 0.46f},
     {0.08f, 0.10f, 0.15f},
     0.0040f,
     {0.0f, 0.0f, 0.0f},
     {0.03f, 0.05f, 0.10f},
     0.30f},
};
// NOLINTEND(readability-magic-numbers)

// The curve in use: the default until a project replaces it.
static const int       ATMO_MAX_KEYS = 32;
static OkAtmosphereKey ATMO_KEYS[ATMO_MAX_KEYS];
static int             ATMO_KEY_COUNT = 0;

static void ensureCurve() {
  if (ATMO_KEY_COUNT > 0) {
    return;
  }
  int n = static_cast<int>(sizeof(ATMO_DEFAULT) / sizeof(ATMO_DEFAULT[0]));
  for (int i = 0; i < n; i++) {
    ATMO_KEYS[i] = ATMO_DEFAULT[i];
  }
  ATMO_KEY_COUNT = n;
}

void OkLighting::setAtmosphereCurve(const OkAtmosphereKey *keys, int count) {
  if (keys == nullptr || count < 2) {
    return;
  }
  count = std::min(count, ATMO_MAX_KEYS);
  for (int i = 0; i < count; i++) {
    ATMO_KEYS[i] = keys[i];
  }
  ATMO_KEY_COUNT = count;
  OkLogger::info("Lighting", "Atmosphere curve replaced (" +
                                 std::to_string(count) + " keys)");
}

int OkLighting::getAtmosphereKeyCount() {
  ensureCurve();
  return ATMO_KEY_COUNT;
}

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
        const char *tbeg = args[0].c_str();
        char       *tend = nullptr;
        double      tval = strtod(tbeg, &tend);
        if (tend == tbeg || *tend != '\0') {
          OkConsole::print("time: not a number");
          return;
        }
        OkLighting::setTimeOfDay(static_cast<float>(tval));
        OkConsole::print("time = " +
                         std::to_string(OkLighting::getTimeOfDay()));
      });
  OkConsole::registerCommand(
      "timescale",
      "timescale [x]: how much faster than real time the clock runs",
      [](const std::vector<std::string> &args) {
        if (args.empty()) {
          OkConsole::print("timescale = " + std::to_string(OkConfig::getFloat(
                                                "lighting.timescale")));
          return;
        }
        const char *sbeg = args[0].c_str();
        char       *send = nullptr;
        double      sval = strtod(sbeg, &send);
        if (send == sbeg || *send != '\0') {
          OkConsole::print("timescale: not a number");
          return;
        }
        OkConfig::setFloat("lighting.timescale", static_cast<float>(sval));
        OkConsole::print("timescale = " + std::to_string(OkConfig::getFloat(
                                              "lighting.timescale")));
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

  // Point-light level: sin(sun elevation) is -_sunDir[1]; ramp 0 -> 1
  // as it falls from +0.05 to -0.05 (through the sunset).
  {
    float sinElev    = -_sunDir[1];
    float level      = (0.05f - sinElev) / 0.10f;
    level            = std::max(level, 0.0f);
    level            = std::min(level, 1.0f);
    _pointLightLevel = level;
  }
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
  ensureCurve();
  float h = std::fmod(hours, 24.0f);
  if (h < 0.0f) {
    h += 24.0f;
  }

  // Find the surrounding keyframes (wrapping around midnight).
  const OkAtmosphereKey *prev = &ATMO_KEYS[ATMO_KEY_COUNT - 1];
  const OkAtmosphereKey *next = &ATMO_KEYS[0];
  float                  span = (24.0f - prev->hour) + next->hour;
  float                  frac = 0.0f;
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
  outFogDensity =
      prev->fogDensity + (next->fogDensity - prev->fogDensity) * frac;
  if (outAmbient != nullptr) {
    *outAmbient = prev->ambient + (next->ambient - prev->ambient) * frac;
  }

  // Sun direction: elevation follows a sine across the 6h..21h daylight
  // arc (below the horizon at night), azimuth sweeps east to west. The
  // vector points FROM the sun TOWARD the scene (ready for lighting).
  // NOLINTBEGIN(readability-magic-numbers)
  float dayFrac = (h - 6.0f) / 15.0f;                   // 0 at 6h, 1 at 21h
  float elev = std::sin(dayFrac * 3.14159265f) * 1.2f;  // radians, peak ~69 deg
  float azim = (dayFrac - 0.5f) * 3.14159265f;          // -90..+90 deg
  if (dayFrac < 0.0f || dayFrac > 1.0f) {
    elev = -0.3f;  // parked below the horizon at night
  }
  // NOLINTEND(readability-magic-numbers)
  float cosE   = std::cos(elev);
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
  int id              = _lightCount;
  _lightPos[id][0]    = x;
  _lightPos[id][1]    = y;
  _lightPos[id][2]    = z;
  _lightColor[id][0]  = r;
  _lightColor[id][1]  = g;
  _lightColor[id][2]  = b;
  _lightRadius[id]    = radius;
  _lightDir[id][0]    = 0.0f;
  _lightDir[id][1]    = -1.0f;
  _lightDir[id][2]    = 0.0f;
  _lightCosCone[id]   = -2.0f;  // omni
  _lightIntensity[id] = 1.0f;
  _lightCount++;
  _lightGeneration++;
  return id;
}

int OkLighting::registerSpotLight(float x, float y, float z, float r, float g,
                                  float b, float radius, float dirX, float dirY,
                                  float dirZ, float coneDeg, float intensity) {
  int id = registerLight(x, y, z, r, g, b, radius);
  if (id < 0) {
    return id;
  }
  float len = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
  if (len < 1e-6f) {
    len  = 1.0f;
    dirY = -1.0f;
  }
  _lightDir[id][0]    = dirX / len;
  _lightDir[id][1]    = dirY / len;
  _lightDir[id][2]    = dirZ / len;
  _lightCosCone[id]   = std::cos(coneDeg * 3.14159265f / 180.0f);
  _lightIntensity[id] = intensity;
  _lightGeneration++;
  return id;
}

void OkLighting::clearLights() {
  _lightCount = 0;
  _lightGeneration++;
}

long OkLighting::getLightGeneration() {
  return _lightGeneration;
}

int OkLighting::getLightCount() {
  return _lightCount;
}

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
  maxN    = std::min(maxN, 8);
  for (int i = 0; i < _lightCount; i++) {
    float dx = _lightPos[i][0] - x;
    float dy = _lightPos[i][1] - y;
    float dz = _lightPos[i][2] - z;
    float d  = std::sqrt(dx * dx + dy * dy + dz * dz);
    float s  = d / (_lightRadius[i] > 1.0f ? _lightRadius[i] : 1.0f);
    // insertion into the small sorted best list
    int at = n;
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

const float *OkLighting::getLightPosition(int idx) {
  return _lightPos[idx];
}

const float *OkLighting::getLightColor(int idx) {
  return _lightColor[idx];
}

float OkLighting::getLightRadius(int idx) {
  return _lightRadius[idx];
}

const float *OkLighting::getLightDirection(int idx) {
  return _lightDir[idx];
}

float OkLighting::getLightCosCone(int idx) {
  return _lightCosCone[idx];
}

float OkLighting::getLightIntensity(int idx) {
  return _lightIntensity[idx];
}

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
      float dx      = (static_cast<float>(x) + 0.5f) / SIZE - 0.5f;
      float dy      = (static_cast<float>(y) + 0.5f) / SIZE - 0.5f;
      float d       = std::sqrt(dx * dx + dy * dy) * 2.0f;  // 0 centre, 1 edge
      float a       = 1.0f - d;
      a             = std::max(a, 0.0f);
      a             = a * a;  // quadratic falloff, soft rim
      int off       = (y * SIZE + x) * 4;
      rgba[off]     = 255;
      rgba[off + 1] = 255;
      rgba[off + 2] = 255;
      rgba[off + 3] = static_cast<unsigned char>(a * 255.0f);
    }
  }
  return OkTextureHandler::getInstance()->createTextureFromRawData(
      "ok_halo", rgba, SIZE, SIZE, 4);
}
