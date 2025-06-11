#ifndef OK_ASSETS_HPP
#define OK_ASSETS_HPP

#include <filesystem>
#include <string>

/**
 * @brief Asset management class for the Okinawa engine.
 *        This class provides centralized asset path management and
 *        common asset loading functionality for shaders, textures, etc.
 *
 *        Assets are organized in the following structure:
 *        - Engine assets: Located in the engine's assets/ directory
 *          - shaders/: Vertex and fragment shaders (.vert.glsl, .frag.glsl)
 *          - textures/: Default textures and materials
 *        - Project assets: Located in the consuming project's assets/ directory
 *          - Custom textures, models, configurations, etc.
 */
class OkAssets {
public:
  // Static class - no instantiation
  OkAssets() = delete;

  // Delete copy and assignment operators
  OkAssets(const OkAssets &)            = delete;
  OkAssets &operator=(const OkAssets &) = delete;

  // Initialization
  static bool initialize();

  // Engine asset paths (built-in shaders, textures)
  static std::filesystem::path
  getEngineAssetPath(const std::string &relativePath);
  static std::filesystem::path getShaderPath(const std::string &shaderName);
  static bool                  shaderExists(const std::string &shaderName);
  static std::string           loadShaderSource(const std::string &shaderName);

  // Project asset paths (user-provided assets)
  static void setProjectAssetRoot(const std::filesystem::path &path);
  static std::filesystem::path
  getProjectAssetPath(const std::string &relativePath);
  static std::filesystem::path getProjectAssetRoot();

  // General utilities
  static bool exists(const std::filesystem::path &path);

private:
  // Internal methods for managing static state
  static std::filesystem::path &getMutableEngineRoot();
  static std::filesystem::path &getMutableProjectRoot();
  static bool                   discoverEngineAssetRoot();
};

#endif  // OK_ASSETS_HPP
