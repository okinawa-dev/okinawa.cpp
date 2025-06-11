#include "logger.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

// Initialize static members
/**
 * @brief Static map to hold log type filters.
 *        If a type is not in the map, it defaults to the global default state.
 */
std::unordered_map<std::string, bool> OkLogger::logTypeFilters;

/**
 * @brief Default state for log type filtering.
 *        If true, all log types are enabled by default.
 *        If false, no log types are enabled by default.
 *        This is overridden by individual type settings.
 */
bool OkLogger::defaultLogTypeEnabled = true;

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
 * @brief Log a message with a specific log level and type filtering.
 * @param level The log level (Info, Warning, Error).
 * @param type The component type for filtering (e.g., "Item", "Core", "Scene").
 * @param message The message to log.
 */
void OkLogger::log(LogLevel level, const std::string &type,
                   const std::string &message) {
  // Check if this log type is enabled (skip filtering for empty type)
  if (!type.empty() && !isLogTypeEnabled(type)) {
    return;
  }

  std::cerr << getLevelColor(level) << getCurrentTimestamp() << " ["
            << getLevelString(level) << "]: ";

  if (!type.empty()) {
    std::cerr << type << " :: ";
  }

  std::cerr << message << RESET_COLOR << '\n';
}

/**
 * @brief Log an info message with type filtering.
 * @param type The component type for filtering.
 * @param message The message to log.
 */
void OkLogger::info(const std::string &type, const std::string &message) {
  log(LogLevel::Info, type, message);
}

/**
 * @brief Log a warning message with type filtering.
 * @param type The component type for filtering.
 * @param message The message to log.
 */
void OkLogger::warning(const std::string &type, const std::string &message) {
  log(LogLevel::Warning, type, message);
}

/**
 * @brief Log an error message with type filtering.
 * @param type The component type for filtering.
 * @param message The message to log.
 */
void OkLogger::error(const std::string &type, const std::string &message) {
  log(LogLevel::Error, type, message);
}

/**
 * @brief Log an info message.
 * @param message The message to log.
 */
void OkLogger::info(const std::string &message) {
  log(LogLevel::Info, "", message);
}

/**
 * @brief Log a warning message.
 * @param message The message to log.
 */
void OkLogger::warning(const std::string &message) {
  log(LogLevel::Warning, "", message);
}

/**
 * @brief Log an error message.
 * @param message The message to log.
 */
void OkLogger::error(const std::string &message) {
  log(LogLevel::Error, "", message);
}

// Configuration methods
/**
 * @brief Enable logging for a specific type.
 * @param type The component type to enable.
 */
void OkLogger::enableLogType(const std::string &type) {
  logTypeFilters[type] = true;
}

/**
 * @brief Disable logging for a specific type.
 * @param type The component type to disable.
 */
void OkLogger::disableLogType(const std::string &type) {
  logTypeFilters[type] = false;
}

/**
 * @brief Check if logging is enabled for a specific type.
 * @param type The component type to check.
 * @return true if logging is enabled for this type, false otherwise.
 */
bool OkLogger::isLogTypeEnabled(const std::string &type) {
  auto it = logTypeFilters.find(type);
  if (it != logTypeFilters.end()) {
    return it->second;
  }

  // If not found in filters, return default behavior
  return defaultLogTypeEnabled;
}

/**
 * @brief Enable logging for all types.
 */
void OkLogger::enableAllLogTypes() {
  defaultLogTypeEnabled = true;
  logTypeFilters.clear();
}

/**
 * @brief Disable logging for all types.
 */
void OkLogger::disableAllLogTypes() {
  defaultLogTypeEnabled = false;
  logTypeFilters.clear();
}
