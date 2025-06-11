#ifndef OK_LOGGER_HPP
#define OK_LOGGER_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

enum class LogLevel : std::uint8_t {
  Info,
  Warning,
  Error
};

class OkLogger {
public:
  // Delete constructor to prevent instantiation
  OkLogger() = delete;

  // Static logging methods with type filtering
  static void info(const std::string &type, const std::string &message);
  static void info(const std::string &message);
  static void warning(const std::string &type, const std::string &message);
  static void warning(const std::string &message);
  static void error(const std::string &type, const std::string &message);
  static void error(const std::string &message);
  static void log(LogLevel level, const std::string &type,
                  const std::string &message);

  // Configuration methods
  static void enableLogType(const std::string &type);
  static void disableLogType(const std::string &type);
  static void setLogTypeEnabled(const std::string &type, bool enabled);
  static bool isLogTypeEnabled(const std::string &type);
  static void enableAllLogTypes();
  static void disableAllLogTypes();

private:
  static std::unordered_map<std::string, bool> logTypeFilters;
  static bool                                  defaultLogTypeEnabled;
};

// Convenience functions for easier use
// clang-format off
// namespace Log {
//   inline void info(const std::string& message) { OkLogger::info(message); }
//   inline void warning(const std::string& message) { OkLogger::warning(message); }
//   inline void error(const std::string& message) { OkLogger::error(message); }
// }
// clang-format on

#endif
