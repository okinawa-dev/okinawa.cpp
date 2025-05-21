#ifndef OK_LOGGER_HPP
#define OK_LOGGER_HPP

#include <cstdint>
#include <string>

enum class LogLevel : std::uint8_t {
  Info,
  Warning,
  Error
};

class OkLogger {
public:
  // Delete constructor to prevent instantiation
  OkLogger() = delete;

  // Static logging methods
  static void info(const std::string &message);
  static void warning(const std::string &message);
  static void error(const std::string &message);
  static void log(LogLevel level, const std::string &message);

private:
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
