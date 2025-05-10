#include "./strings.hpp"

std::string OkStrings::trim(const std::string &str) {
  size_t first = str.find_first_not_of(" \0");
  size_t last  = str.find_last_not_of(" \0");

  if (first == std::string::npos || last == std::string::npos) {
    return "";  // String is empty or contains only spaces/nulls
  }

  return str.substr(first, (last - first + 1));
}

std::string OkStrings::trimRight(const std::string &str) {
  size_t last = str.find_last_not_of(" \0");

  if (last == std::string::npos) {
    return "";  // String is empty or contains only spaces/nulls
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
