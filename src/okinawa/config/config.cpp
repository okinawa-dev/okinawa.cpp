#include "config.hpp"
#include "../utils/logger.hpp"
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
  intValues["window.width"]     = 800;
  intValues["window.height"]    = 600;
  stringValues["window.title"]  = "okinawa";

  // Performance settings
  intValues["fps"] = 60;

  // OpenGL settings
  intValues["opengl.infolog.size"] = 512;

  // Calculate time per frame from FPS
  float timePerFrame = 1000.0f / 60.0f;  // Using hardcoded FPS value
  floatValues["graphics.time-per-frame"] = timePerFrame;

  // Size (half-extent, metres) of the camera gizmo drawn for non-active cameras.
  floatValues["camera.gizmo-size"] = 0.25f;

  // GUI settings: grid cell size in logical pixels, global UI scale
  // (0 = resolve automatically from the monitor content scale), field of
  // view of the calibrated GUI camera (degrees; the oblique-element knob)
  // and the debug grid overlay.
  intValues["gui.grid.size"]    = 20;
  floatValues["gui.scale"]      = 0.0f;
  floatValues["gui.fov"]        = 35.0f;
  boolValues["gui.debug.grid"]  = false;
  // NOLINTEND(readability-magic-numbers)
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
