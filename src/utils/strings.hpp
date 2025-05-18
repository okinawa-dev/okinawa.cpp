#ifndef OK_STRINGS_HPP
#define OK_STRINGS_HPP

#include <cstring>
#include <string>

/**
 * @brief Static class providing string manipulation utilities.
 * @note All trim functions use the C++ standard library's definition of
 * whitespace, which includes space, tab, newline, carriage return, form feed,
 * and vertical tab. Null characters are not considered whitespace.
 */
class OkStrings {
public:
  // Static class - no instantiation
  OkStrings() = delete;

  // String operations
  static std::string trim(const std::string &str);       // Trim both ends
  static std::string trimRight(const std::string &str);  // Trim right end only
  // For fixed-length char arrays
  static std::string trimFixedString(const std::string &str, size_t maxLen);
  static std::string toUpper(const std::string &str);
  static std::string toLower(const std::string &str);

private:
};

#endif
