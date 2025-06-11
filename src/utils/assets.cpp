#include "assets.hpp"
#include "../utils/files.hpp"
#include "../utils/logger.hpp"
#include <filesystem>
#include <string>

/**
 * @brief Initialize the asset management system.
 *        This method discovers the engine asset root automatically
 *        and sets up the asset management for both engine and project assets.
 * @return True if initialization was successful, false otherwise.
 */
bool OkAssets::initialize() {
  OkLogger::info("Assets", "Initializing asset management system...");

  if (!discoverEngineAssetRoot()) {
    OkLogger::error("Assets", "Failed to discover engine asset root");
    return false;
  }

  OkLogger::info("Assets",
                 "Engine asset root: " + getMutableEngineRoot().string());
  return true;
}

/**
 * @brief Get the internal mutable reference to the engine asset root.
 * @return Reference to the static engine asset root path.
 */
std::filesystem::path &OkAssets::getMutableEngineRoot() {
  static std::filesystem::path engineRoot;
  return engineRoot;
}

/**
 * @brief Get the internal mutable reference to the project asset root.
 * @return Reference to the static project asset root path.
 */
std::filesystem::path &OkAssets::getMutableProjectRoot() {
  static std::filesystem::path projectRoot;
  return projectRoot;
}

/**
 * @brief Automatically discover the engine asset root directory.
 *        This method searches for the src/shaders directory starting from
 *        the current working directory and moving up the directory tree.
 * @return True if the engine asset root was found, false otherwise.
 */
bool OkAssets::discoverEngineAssetRoot() {
  std::filesystem::path currentPath = std::filesystem::current_path();

  // Search up the directory tree for the engine structure
  for (int i = 0; i < 5; i++) {
    std::filesystem::path candidate = currentPath / "assets" / "shaders";

    if (std::filesystem::exists(candidate) &&
        std::filesystem::is_directory(candidate)) {
      getMutableEngineRoot() = currentPath;
      return true;
    }

    // Also check for okinawa.cpp specific structure
    candidate = currentPath / "okinawa.cpp" / "assets" / "shaders";
    if (std::filesystem::exists(candidate) &&
        std::filesystem::is_directory(candidate)) {
      getMutableEngineRoot() = currentPath / "okinawa.cpp";
      return true;
    }

    currentPath = currentPath.parent_path();
    if (currentPath == currentPath.parent_path()) {
      break;  // Reached filesystem root
    }
  }

  return false;
}

/**
 * @brief Get the path to an engine asset (shader, default texture, etc.).
 * @param relativePath Path relative to the engine asset root (src/).
 * @return Absolute path to the engine asset.
 */
std::filesystem::path
OkAssets::getEngineAssetPath(const std::string &relativePath) {
  return getMutableEngineRoot() / relativePath;
}

/**
 * @brief Get the path to a shader file.
 * @param shaderName Name of the shader file (e.g., "vertexshader.vert.glsl").
 * @return Absolute path to the shader file.
 */
std::filesystem::path OkAssets::getShaderPath(const std::string &shaderName) {
  return getEngineAssetPath("assets/shaders/" + shaderName);
}

/**
 * @brief Check if a shader file exists.
 * @param shaderName Name of the shader file to check.
 * @return True if the shader exists, false otherwise.
 */
bool OkAssets::shaderExists(const std::string &shaderName) {
  return exists(getShaderPath(shaderName));
}

/**
 * @brief Load shader source code from file.
 * @param shaderName Name of the shader file to load.
 * @return The shader source code as a string, empty if failed.
 */
std::string OkAssets::loadShaderSource(const std::string &shaderName) {
  std::filesystem::path shaderPath = getShaderPath(shaderName);

  if (!exists(shaderPath)) {
    OkLogger::error("Assets", "Shader file not found: " + shaderPath.string());
    return "";
  }

  std::string source = OkFiles::readFile(shaderPath.string());
  if (source.empty()) {
    OkLogger::error("Assets", "Failed to load shader: " + shaderName);
  }

  return source;
}

/**
 * @brief Set the root directory for project assets.
 *        This is typically the assets/ directory of the consuming project.
 * @param path The absolute path to the project assets directory.
 */
void OkAssets::setProjectAssetRoot(const std::filesystem::path &path) {
  getMutableProjectRoot() = path;
  OkLogger::info("Assets", "Project asset root set to: " + path.string());
}

/**
 * @brief Get the path to a project asset.
 * @param relativePath Path relative to the project asset root.
 * @return Absolute path to the project asset.
 */
std::filesystem::path
OkAssets::getProjectAssetPath(const std::string &relativePath) {
  return getMutableProjectRoot() / relativePath;
}

/**
 * @brief Get the current project asset root directory.
 * @return The current project asset root path.
 */
std::filesystem::path OkAssets::getProjectAssetRoot() {
  return getMutableProjectRoot();
}

/**
 * @brief Check if a file or directory exists.
 * @param path The path to check.
 * @return True if the path exists, false otherwise.
 */
bool OkAssets::exists(const std::filesystem::path &path) {
  return std::filesystem::exists(path);
}
