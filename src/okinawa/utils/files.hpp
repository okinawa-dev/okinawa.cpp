#ifndef OK_FILES_HPP
#define OK_FILES_HPP

#include <filesystem>
#include <string>

class OkFiles {
public:
  // Static class - no instantiation
  OkFiles() = delete;

  // File operations
  static std::string readFile(const std::string &filename);

  /**
   * @brief The directory the running executable sits in.
   *
   *        Asked for by anything that has to find its own files when the
   *        working directory is not its own: an application started from
   *        somewhere else, a double-clicked bundle (macOS hands one a
   *        working directory of `/`), a service. Resolved from the
   *        operating system rather than from argv[0], which a caller can
   *        set to anything.
   *
   * @return An empty path when the platform will not say.
   */
  static std::filesystem::path executableDirectory();

private:
};

#endif
