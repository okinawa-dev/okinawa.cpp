#ifndef OK_UTILS_HPP
#define OK_UTILS_HPP

#include <string>

class OkFiles {
public:
  // Static class - no instantiation
  OkFiles() = delete;

  // File operations
  static std::string readFile(const std::string &filename);

private:
};

#endif
