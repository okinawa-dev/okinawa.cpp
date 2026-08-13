#include "config.hpp"
#include "../utils/logger.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <string>

/**
 * @brief OkConfig constructor initializes default values for the configuration.
 *       The constructor is private to enforce singleton pattern.
 */
OkConfig::OkConfig() {
  setDefaults();
}

/**
 * @brief Populate the configuration with its default values.
 */
void OkConfig::setDefaults() {
  // NOLINTBEGIN(readability-magic-numbers)
  // Graphics settings
  boolValues["graphics.wireframe"]   = false;
  boolValues["graphics.textures"]    = true;
  boolValues["graphics.drawCameras"] = true;

  // Window settings
  intValues["window.width"]    = 800;
  intValues["window.height"]   = 600;
  stringValues["window.title"] = "okinawa";

  // Performance settings
  intValues["fps"] = 60;

  // OpenGL settings
  intValues["opengl.infolog.size"] = 512;

  // Calculate time per frame from FPS
  float timePerFrame = 1000.0f / 60.0f;  // Using hardcoded FPS value
  floatValues["graphics.time-per-frame"] = timePerFrame;

  // Size (half-extent, metres) of the camera gizmo drawn for non-active
  // cameras.
  floatValues["camera.gizmo-size"] = 0.25f;

  // GUI settings: grid cell size in logical pixels, global UI scale
  // (0 = resolve automatically from the monitor content scale), field of
  // view of the calibrated GUI camera (degrees; the oblique-element knob)
  // and the debug grid overlay.
  intValues["gui.grid.size"] = 20;
  // Lighting: day-clock hour (0-24) and how much faster than real time
  // the clock runs (30 = a full day in 48 real minutes; 0 freezes it).
  floatValues["lighting.time"]      = 12.0f;
  floatValues["lighting.timescale"] = 30.0f;
  // Swap interval: 1 waits for the display refresh (steady, no tearing),
  // 0 runs free. Turn it off to MEASURE: with vsync on, frame times can
  // only land on multiples of the refresh, so a change that costs 2 ms
  // shows up as either nothing at all or a drop to half the frame rate.
  intValues["render.vsync"]  = 1;
  boolValues["lighting.fog"] = true;
  // Fog thins with altitude: the atmosphere curve's density applies at
  // "base", and every "height" metres above it the air is e times
  // thinner. A very large height makes the air uniform, which is plain
  // distance fog. Projects with terrain should set base to their ground
  // level, since world Y is not altitude above the ground.
  floatValues["lighting.fog.height"] = 25.0f;
  floatValues["lighting.fog.base"]   = 0.0f;
  floatValues["gui.scale"]           = 0.0f;
  floatValues["gui.fov"]             = 35.0f;
  boolValues["gui.debug.grid"]       = false;
  // NOLINTEND(readability-magic-numbers)
}

/**
 * @brief Every key starting with `prefix`, across the four typed maps,
 *        sorted alphabetically.
 */
std::vector<std::string>
OkConfig::getKeysWithPrefix(const std::string &prefix) {
  OkConfig                &config = getConfig();
  std::vector<std::string> keys;
  for (const auto &kv : config.intValues) {
    if (kv.first.rfind(prefix, 0) == 0) {
      keys.push_back(kv.first);
    }
  }
  for (const auto &kv : config.floatValues) {
    if (kv.first.rfind(prefix, 0) == 0) {
      keys.push_back(kv.first);
    }
  }
  for (const auto &kv : config.boolValues) {
    if (kv.first.rfind(prefix, 0) == 0) {
      keys.push_back(kv.first);
    }
  }
  for (const auto &kv : config.stringValues) {
    if (kv.first.rfind(prefix, 0) == 0) {
      keys.push_back(kv.first);
    }
  }
  std::sort(keys.begin(), keys.end());
  return keys;
}

/**
 * @brief True when the key exists in any of the typed maps.
 */
bool OkConfig::hasKey(const std::string &key) {
  OkConfig &config = getConfig();
  return config.intValues.count(key) > 0 || config.floatValues.count(key) > 0 ||
         config.boolValues.count(key) > 0 || config.stringValues.count(key) > 0;
}

/**
 * @brief The key's value formatted as a string, whatever its type.
 */
std::string OkConfig::getValueAsString(const std::string &key) {
  OkConfig &config = getConfig();
  if (config.boolValues.count(key) > 0) {
    return config.boolValues[key] ? "true" : "false";
  }
  if (config.intValues.count(key) > 0) {
    return std::to_string(config.intValues[key]);
  }
  if (config.floatValues.count(key) > 0) {
    return std::to_string(config.floatValues[key]);
  }
  if (config.stringValues.count(key) > 0) {
    return config.stringValues[key];
  }
  return "<unset>";
}

/**
 * @brief Reset the configuration back to its default values.
 *        Clears any values set at runtime (or by tests) and re-applies the
 *        defaults, so callers start from a known, clean state.
 */
void OkConfig::reset() {
  OkConfig &config = getConfig();
  config.intValues.clear();
  config.floatValues.clear();
  config.boolValues.clear();
  config.stringValues.clear();
  config.setDefaults();
}

/**
 * @brief OkConfig singleton instance getter.
 *        This method returns a reference to the static instance of OkConfig.
 * @return Reference to the OkConfig instance.
 */
OkConfig &OkConfig::getConfig() {
  static OkConfig instance;
  return instance;
}

/**
 * @brief Set an integer value in the configuration.
 * @param key   The key for the configuration value.
 * @param value The integer value to set.
 */
void OkConfig::setInt(const std::string &key, int value) {
  getConfig().intValues[key] = value;
}

/**
 * @brief Set a float value in the configuration.
 * @param key   The key for the configuration value.
 * @param value The float value to set.
 */
void OkConfig::setFloat(const std::string &key, float value) {
  getConfig().floatValues[key] = value;
}

// Text to number, refusing to invent one.
//
// `atof` and `atoi` answer 0 for anything they cannot read, so a typo in a
// config file or a console line silently sets the value to zero and the
// only clue is behaviour. These say whether the text WAS a number, and the
// caller decides what to do when it was not.
static bool parseFloat(const std::string &text, float *out) {
  const char *begin = text.c_str();
  char       *end   = nullptr;
  double      v     = strtod(begin, &end);
  if (end == begin || *end != '\0') {
    return false;
  }
  *out = static_cast<float>(v);
  return true;
}

static bool parseInt(const std::string &text, int *out) {
  const char *begin = text.c_str();
  char       *end   = nullptr;
  long        v     = strtol(begin, &end, 10);
  if (end == begin || *end != '\0') {
    return false;
  }
  *out = static_cast<int>(v);
  return true;
}

void OkConfig::setFromString(const std::string &key, const std::string &val) {
  OkConfig &cfg = getConfig();
  if (cfg.floatValues.find(key) != cfg.floatValues.end()) {
    float f = 0.0f;
    if (parseFloat(val, &f)) {
      setFloat(key, f);
    }
    return;
  }
  if (cfg.boolValues.find(key) != cfg.boolValues.end()) {
    setBool(key, val == "true" || val == "1");
    return;
  }
  if (cfg.intValues.find(key) != cfg.intValues.end()) {
    int i = 0;
    if (parseInt(val, &i)) {
      setInt(key, i);
    }
    return;
  }
  if (cfg.stringValues.find(key) != cfg.stringValues.end()) {
    setString(key, val);
    return;
  }
  // Unknown key: guess the type from the text shape.
  if (val == "true" || val == "false") {
    setBool(key, val == "true");
  } else if (val.find('.') != std::string::npos) {
    float f = 0.0f;
    if (parseFloat(val, &f)) {
      setFloat(key, f);
    } else {
      setString(key, val);
    }
  } else if (!val.empty() &&
             (isdigit(static_cast<unsigned char>(val[0])) || val[0] == '-')) {
    int i = 0;
    if (parseInt(val, &i)) {
      setInt(key, i);
    } else {
      setString(key, val);
    }
  } else {
    setString(key, val);
  }
}

/**
 * @brief Set a boolean value in the configuration.
 * @param key   The key for the configuration value.
 * @param value The boolean value to set.
 */
void OkConfig::setBool(const std::string &key, bool value) {
  getConfig().boolValues[key] = value;
}

/**
 * @brief Set a string value in the configuration.
 * @param key   The key for the configuration value.
 * @param value The string value to set.
 */
void OkConfig::setString(const std::string &key, const std::string &value) {
  getConfig().stringValues[key] = value;
}

/**
 * @brief Get an integer value from the configuration.
 * @param key The key for the configuration value.
 * @return The integer value associated with the key.
 */
int OkConfig::getInt(const std::string &key) {
  try {
    return getConfig().intValues.at(key);
  } catch (const std::exception &e) {
    OkLogger::error("Config", "Failed to get int value for key: " + key);
    return 0;
  }
}

/**
 * @brief Get a float value from the configuration.
 * @param key The key for the configuration value.
 * @return The float value associated with the key.
 */
float OkConfig::getFloat(const std::string &key) {
  try {
    return getConfig().floatValues.at(key);
  } catch (const std::exception &e) {
    OkLogger::error("Config", "Failed to get float value for key: " + key);
    return 0.0f;
  }
}

/**
 * @brief Get a boolean value from the configuration.
 * @param key The key for the configuration value.
 * @return The boolean value associated with the key.
 */
bool OkConfig::getBool(const std::string &key) {
  try {
    return getConfig().boolValues.at(key);
  } catch (const std::exception &e) {
    OkLogger::error("Config", "Failed to get bool value for key: " + key);
    return false;
  }
}

/**
 * @brief Get a string value from the configuration.
 * @param key The key for the configuration value.
 * @return The string value associated with the key, or empty if missing.
 */
std::string OkConfig::getString(const std::string &key) {
  try {
    return getConfig().stringValues.at(key);
  } catch (const std::exception &e) {
    OkLogger::error("Config", "Failed to get string value for key: " + key);
    return "";
  }
}
