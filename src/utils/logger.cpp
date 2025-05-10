#include "logger.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace {
  const char *RESET_COLOR   = "\x1b[0m";
  const char *INFO_COLOR    = "\x1b[32m";  // Green
  const char *WARNING_COLOR = "\x1b[33m";  // Yellow
  const char *ERROR_COLOR   = "\x1b[31m";  // Red

  /**
   * @brief Get the string representation of a log level.
   * @param level The log level (Info, Warning, Error).
   * @return The string representation of the log level.
   *        Returns "UNKNOWN" for any other level.
   */
  const char *getLevelString(LogLevel level) {
    switch (level) {
      case LogLevel::Info:
        return "INFO";
      case LogLevel::Warning:
        return "WARNING";
      case LogLevel::Error:
        return "ERROR";
      default:
        return "UNKNOWN";
    }
  }

  /**
   * @brief Get the current timestamp in HH:MM:SS format.
   * @return The current timestamp as a string.
   */
  std::string getCurrentTimestamp() {
    auto              now  = std::chrono::system_clock::now();
    auto              time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    return ss.str();
  }

  /**
   * @brief Get the color code for a specific log level.
   * @param level The log level (Info, Warning, Error).
   * @return The color code as a string.
   */
  const char *getLevelColor(LogLevel level) {
    switch (level) {
      case LogLevel::Info:
        return INFO_COLOR;
      case LogLevel::Warning:
        return WARNING_COLOR;
      case LogLevel::Error:
        return ERROR_COLOR;
      default:
        return RESET_COLOR;
    }
  }
}  // namespace

/**
 * @brief Log a message with a specific log level.
 * @param level The log level (Info, Warning, Error).
 * @param message The message to log.
 */
void OkLogger::log(LogLevel level, const std::string &message) {
  std::cerr << getLevelColor(level) << getCurrentTimestamp() << " ["
            << getLevelString(level) << "]: " << message << RESET_COLOR << '\n';
}

/**
 * @brief Log an info message.
 * @param message The message to log.
 */
void OkLogger::info(const std::string &message) {
  log(LogLevel::Info, message);
}

/**
 * @brief Log a warning message.
 * @param message The message to log.
 */
void OkLogger::warning(const std::string &message) {
  log(LogLevel::Warning, message);
}

/**
 * @brief Log an error message.
 * @param message The message to log.
 */
void OkLogger::error(const std::string &message) {
  log(LogLevel::Error, message);
}
