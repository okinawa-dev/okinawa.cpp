#include "files.hpp"
#include "logger.hpp"
#include <fstream>
#include <ios>
#include <sstream>
#include <string>

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
