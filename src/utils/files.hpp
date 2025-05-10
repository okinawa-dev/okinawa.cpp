#ifndef OK_FILES_HPP
#define OK_FILES_HPP

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
