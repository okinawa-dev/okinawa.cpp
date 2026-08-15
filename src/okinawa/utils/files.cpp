#include "files.hpp"

#ifdef __APPLE__
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "logger.hpp"
#include <fstream>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Read the contents of a file into a string.
 * @param filename The name of the file to read.
 * @return The contents of the file as a string.
 *         Returns an empty string if the file cannot be opened.
 */
std::string OkFiles::readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);

  if (!file.is_open()) {
    OkLogger::error("Utils", "Failed to open file: " + filename);
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();

  return buffer.str();
}

std::filesystem::path OkFiles::executableDirectory() {
  std::error_code err;

#ifdef __APPLE__
  // Ask once for the length, then for the path: the first call fails on
  // purpose and reports how much room it wants.
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::vector<char> buffer(size + 1, '\0');
  if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
    return std::filesystem::path();
  }
  std::filesystem::path exe = std::filesystem::canonical(buffer.data(), err);
#elif defined(_WIN32)
  std::vector<wchar_t> buffer(MAX_PATH);
  DWORD len = GetModuleFileNameW(nullptr, buffer.data(),
                                 static_cast<DWORD>(buffer.size()));
  if (len == 0) {
    return std::filesystem::path();
  }
  std::filesystem::path exe =
      std::filesystem::canonical(std::wstring(buffer.data(), len), err);
#else
  std::filesystem::path exe = std::filesystem::canonical("/proc/self/exe", err);
#endif

  if (err) {
    return std::filesystem::path();
  }
  return exe.parent_path();
}
