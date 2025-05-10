#include "./strings.hpp"

std::string OkStrings::trim(const std::string &str) {
  // Match C++ standard library whitespace definition
  static const std::string whitespace = " \t\n\r\f\v\0";
  size_t                   first      = str.find_first_not_of(whitespace);
  size_t                   last       = str.find_last_not_of(whitespace);

  if (first == std::string::npos || last == std::string::npos) {
    return "";  // String is empty or contains only whitespace
  }

  return str.substr(first, (last - first + 1));
}

std::string OkStrings::trimRight(const std::string &str) {
  // Match C++ standard library whitespace definition
  static const std::string whitespace = " \t\n\r\f\v\0";
  size_t                   last       = str.find_last_not_of(whitespace);

  if (last == std::string::npos) {
    return "";  // String is empty or contains only whitespace
  }

  return str.substr(0, last + 1);
}

std::string OkStrings::trimFixedString(const char *str, size_t maxLen) {
  std::string result(str, strnlen(str, maxLen));
  return trimRight(result);
}

std::string OkStrings::toUpper(const std::string &str) {
  std::string result = str;
  for (char &c : result) {
    c = static_cast<char>(std::toupper(c));
  }
  return result;
}

std::string OkStrings::toLower(const std::string &str) {
  std::string result = str;
  for (char &c : result) {
    c = static_cast<char>(std::tolower(c));
  }
  return result;
}
