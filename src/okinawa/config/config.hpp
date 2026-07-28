#ifndef OK_CONFIG_HPP
#define OK_CONFIG_HPP

#include <string>
#include <unordered_map>
#include <vector>

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
  static void setString(const std::string &key, const std::string &value);

  static int         getInt(const std::string &key);
  static float       getFloat(const std::string &key);
  static bool        getBool(const std::string &key);
  static std::string getString(const std::string &key);

  // Every key starting with `prefix` (all four typed maps), sorted.
  static std::vector<std::string> getKeysWithPrefix(const std::string &prefix);

  // Whether the key exists in any typed map, and its value formatted as a
  // string ("<unset>" when missing). The console's `get` uses both.
  static bool        hasKey(const std::string &key);
  static std::string getValueAsString(const std::string &key);

  // Reset every value back to the defaults, discarding anything set at
  // runtime. Mainly useful to isolate global state between unit tests.
  static void reset();

private:
  OkConfig();  // Private constructor with initialization

  // Populate the configuration with its default values.
  void setDefaults();

  // Separate maps for each type
  std::unordered_map<std::string, int>         intValues;
  std::unordered_map<std::string, float>       floatValues;
  std::unordered_map<std::string, bool>        boolValues;
  std::unordered_map<std::string, std::string> stringValues;
};

#endif
