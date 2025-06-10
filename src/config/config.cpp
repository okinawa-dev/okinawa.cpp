#include "config.hpp"
#include "../utils/logger.hpp"
#include <exception>
#include <string>

/**
 * @brief OkConfig constructor initializes default values for the configuration.
 *       The constructor is private to enforce singleton pattern.
 */
OkConfig::OkConfig() {
  // NOLINTBEGIN(readability-magic-numbers)
  // Graphics settings
  boolValues["graphics.wireframe"]   = false;
  boolValues["graphics.textures"]    = true;
  boolValues["graphics.drawCameras"] = true;

  // Window settings
  intValues["window.width"]  = 800;
  intValues["window.height"] = 600;

  // Performance settings
  intValues["fps"] = 60;

  // OpenGL settings
  intValues["opengl.infolog.size"] = 512;

  // Calculate time per frame from FPS
  float timePerFrame = 1000.0f / 60.0f;  // Using hardcoded FPS value
  floatValues["graphics.time-per-frame"] = timePerFrame;
  // NOLINTEND(readability-magic-numbers)
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
 * @brief Get an integer value from the configuration.
 * @param key The key for the configuration value.
 * @return The integer value associated with the key.
 */
int OkConfig::getInt(const std::string &key) {
  try {
    return getConfig().intValues.at(key);
  } catch (const std::exception &e) {
    OkLogger::error("Config :: Failed to get int value for key: " + key);
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
    OkLogger::error("Config :: Failed to get float value for key: " + key);
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
    OkLogger::error("Config :: Failed to get bool value for key: " + key);
    return false;
  }
}
