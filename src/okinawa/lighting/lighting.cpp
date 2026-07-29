#include "lighting.hpp"
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

// The atmosphere curve: one keyframe per anchor hour, linearly interpolated
// around the clock (the last segment wraps 23 -> 5). The palette follows the
// agreed look: neutral day, warm sunset, cold teal night.
// NOLINTBEGIN(readability-magic-numbers)
struct OkAtmoKey {
  float hour;
  float tint[3];      // scene tint (multiplies albedo)
  float fog[3];       // fog colour
  float density;      // exponential fog density (per metre)
  float sun[3];       // sun colour (consumed by the directional stage later)
};

static const OkAtmoKey ATMO_KEYS[] = {
    // deep night: cold teal soak, dense milky haze
    {0.0f, {0.30f, 0.40f, 0.48f}, {0.10f, 0.16f, 0.20f}, 0.0060f,
     {0.0f, 0.0f, 0.0f}},
    {5.0f, {0.30f, 0.40f, 0.48f}, {0.10f, 0.16f, 0.20f}, 0.0060f,
     {0.0f, 0.0f, 0.0f}},
    // dawn: haze warms and thins
    {7.0f, {0.85f, 0.75f, 0.70f}, {0.70f, 0.60f, 0.55f}, 0.0035f,
     {1.0f, 0.75f, 0.55f}},
    // day: neutral light, light blue distance haze
    {9.0f, {1.00f, 1.00f, 1.00f}, {0.72f, 0.78f, 0.85f}, 0.0018f,
     {1.0f, 0.98f, 0.92f}},
    {17.0f, {1.00f, 1.00f, 1.00f}, {0.72f, 0.78f, 0.85f}, 0.0018f,
     {1.0f, 0.98f, 0.92f}},
    // sunset: everything warms, haze turns amber-pink
    {20.0f, {1.00f, 0.72f, 0.52f}, {0.75f, 0.50f, 0.42f}, 0.0032f,
     {1.0f, 0.55f, 0.30f}},
    // dusk into night: the teal takes over
    {22.0f, {0.42f, 0.48f, 0.55f}, {0.16f, 0.22f, 0.27f}, 0.0050f,
     {0.2f, 0.15f, 0.15f}},
    {23.0f, {0.30f, 0.40f, 0.48f}, {0.10f, 0.16f, 0.20f}, 0.0060f,
     {0.0f, 0.0f, 0.0f}},
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
  evaluate(getTimeOfDay(), _tint, _fogColor, _fogDensity, _sunColor, _sunDir);
}

/**
 * @brief Interpolate the atmosphere curve at an arbitrary hour. The sun
 *        direction comes from the hour itself (elevation follows a sine
 *        over the 6h..21h daylight arc, azimuth sweeps east to west).
 */
void OkLighting::evaluate(float hours, float outTint[3], float outFogColor[3],
                          float &outFogDensity, float outSunColor[3],
                          float outSunDir[3]) {
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
  }
  outFogDensity = prev->density + (next->density - prev->density) * frac;

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
