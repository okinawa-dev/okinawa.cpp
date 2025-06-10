#include "textures.hpp"
#include "../utils/logger.hpp"
#include "item/texture.hpp"
#include <map>
#include <string>
#include <vector>

OkTextureHandler *OkTextureHandler::instance = nullptr;

/**
 * @brief Constructor for the OkTextureHandler class.
 *        This class is a singleton that manages textures.
 */
OkTextureHandler::OkTextureHandler() = default;

/**
 * @brief Destructor for the OkTextureHandler class.
 *        This method cleans up all textures and clears the map.
 */
OkTextureHandler::~OkTextureHandler() {
  cleanup();
}

/**
 * @brief Get the singleton instance of the OkTextureHandler.
 *        This method creates the instance if it doesn't exist.
 * @return Pointer to the OkTextureHandler instance.
 */
OkTextureHandler *OkTextureHandler::getInstance() {
  if (!instance) {
    instance = new OkTextureHandler();
  }

  return instance;
}

/**
 * @brief Get a texture by name.
 *        This method retrieves a texture from the map and increments its
 *        reference count.
 * @param name The name of the texture.
 * @return Pointer to the OkTexture if found, nullptr otherwise.
 *         The reference count is incremented.
 *         The caller is responsible for managing the texture's lifetime.
 *         If the texture is not found, nullptr is returned.
 */
OkTexture *OkTextureHandler::getTexture(const std::string &name) {
  std::map<std::string, TextureEntry>::iterator it = textureMap.find(name);

  if (it != textureMap.end()) {
    it->second.refCount++;
    return it->second.texture;
  }

  return nullptr;
}

/**
 * @brief Create a texture from a file.
 *        This method checks if the texture already exists in the map.
 *        If it does, it increments the reference count and returns the texture.
 *        If it doesn't, it creates a new texture from the file and adds it to
 *        the map with a reference count of 1.
 * @param path The path to the texture file.
 * @return Pointer to the OkTexture if created successfully, nullptr otherwise.
 */
OkTexture *OkTextureHandler::createTextureFromFile(const std::string &path) {
  // First check if it already exists
  std::map<std::string, TextureEntry>::iterator it = textureMap.find(path);

  if (it != textureMap.end()) {
    it->second.refCount++;
    return it->second.texture;
  }

  // Create new texture
  OkTexture *texture = new OkTexture(path);
  if (!texture->isLoaded()) {
    delete texture;
    return nullptr;
  }

  // Add to map with reference count 1
  TextureEntry entry;
  entry.texture    = texture;
  entry.refCount   = 1;
  textureMap[path] = entry;

  OkLogger::info("TextureHandler :: Created texture '" + path + "' from file");
  return texture;
}

/**
 * @brief Create a texture from raw data.
 *        This method checks if the texture already exists in the map.
 *        If it does, it increments the reference count and returns the texture.
 *        If it doesn't, it creates a new texture from the raw data and adds it
 *        to the map with a reference count of 1.
 * @param name    The name of the texture.
 * @param data    Pointer to the raw texture data.
 * @param width   The width of the texture.
 * @param height  The height of the texture.
 * @param channels The number of channels in the texture data.
 * @return Pointer to the OkTexture if created successfully, nullptr otherwise.
 */
OkTexture *OkTextureHandler::createTextureFromRawData(const std::string   &name,
                                                      const unsigned char *data,
                                                      int width, int height,
                                                      int channels) {
  // First check if it already exists
  std::map<std::string, TextureEntry>::iterator it = textureMap.find(name);
  if (it != textureMap.end()) {
    it->second.refCount++;
    return it->second.texture;
  }

  // Create new texture
  OkTexture *texture = new OkTexture(data, width, height, channels);
  if (!texture->isLoaded()) {
    delete texture;
    return nullptr;
  }

  // Add to map with reference count 1
  TextureEntry entry;
  entry.texture    = texture;
  entry.refCount   = 1;
  textureMap[name] = entry;

  OkLogger::info("TextureHandler :: Created texture '" + name +
                 "' from raw data (" + std::to_string(width) + "x" +
                 std::to_string(height) + ")");

  return texture;
}

/**
 * @brief Add a reference to a texture.
 *        This method increments the reference count of the texture.
 * @param name The name of the texture.
 */
void OkTextureHandler::addReference(const std::string &name) {
  std::map<std::string, TextureEntry>::iterator it = textureMap.find(name);
  if (it != textureMap.end()) {
    it->second.refCount++;
  }
}

/**
 * @brief Remove a reference to a texture.
 *        This method decrements the reference count of the texture.
 *        If the reference count reaches zero, the texture is deleted.
 *        If the texture is not found, no action is taken.
 * @param name The name of the texture.
 */
void OkTextureHandler::removeReference(const std::string &name) {
  std::map<std::string, TextureEntry>::iterator it = textureMap.find(name);

  if (it != textureMap.end()) {
    it->second.refCount--;

    if (it->second.refCount <= 0) {
      OkLogger::info("TextureHandler :: Removing texture: " + name);
      delete it->second.texture;
      textureMap.erase(it);
    }
  }
}

/**
 * @brief Cleanup all textures.
 *        This method deletes all textures in the map and clears the map.
 */
void OkTextureHandler::cleanup() {
  for (const auto &entry : textureMap) {
    delete entry.second.texture;
  }

  textureMap.clear();
}

/**
 * @brief Get the names of all textures.
 *        This method returns a vector of strings containing the names of all
 *        textures in the map.
 * @return Vector of texture names.
 */
std::vector<std::string> OkTextureHandler::getTextureNames() const {
  std::vector<std::string> names;
  names.reserve(textureMap.size());

  for (const auto &entry : textureMap) {
    names.push_back(entry.first);
  }

  return names;
}
