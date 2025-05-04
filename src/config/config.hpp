#ifndef OK_CONFIG_HPP
#define OK_CONFIG_HPP

#include <string>
#include <unordered_map>

class OkConfig {
public:
  static OkConfig &getConfig();

  // Delete copy and assignment operators
  OkConfig(const OkConfig &)            = delete;
  OkConfig &operator=(const OkConfig &) = delete;

  // Specific getters and setters
  static void setInt(const std::string &key, int value);
  static void setFloat(const std::string &key, float value);
  static void setBool(const std::string &key, bool value);

  static int   getInt(const std::string &key);
  static float getFloat(const std::string &key);
  static bool  getBool(const std::string &key);

private:
  OkConfig();  // Private constructor with initialization

  // Separate maps for each type
  std::unordered_map<std::string, int>   intValues;
  std::unordered_map<std::string, float> floatValues;
  std::unordered_map<std::string, bool>  boolValues;
};

#endif
