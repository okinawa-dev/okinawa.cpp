#include "textures.hpp"
#include "../utils/logger.hpp"

OkTextureHandler *OkTextureHandler::instance = nullptr;

OkTextureHandler::OkTextureHandler() {}

OkTextureHandler *OkTextureHandler::getInstance() {
  if (!instance) {
    instance = new OkTextureHandler();
  }
  return instance;
}

OkTexture *OkTextureHandler::getTexture(const std::string &name) {
  std::map<std::string, TextureEntry>::iterator it = textureMap.find(name);
  if (it != textureMap.end()) {
    it->second.refCount++;
    return it->second.texture;
  }
  return nullptr;
}

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

  return texture;
}

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

  OkLogger::info("TextureHandler :: Created new texture: " + name);
  return texture;
}

void OkTextureHandler::addReference(const std::string &name) {
  std::map<std::string, TextureEntry>::iterator it = textureMap.find(name);
  if (it != textureMap.end()) {
    it->second.refCount++;
  }
}

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

void OkTextureHandler::cleanup() {
  for (std::map<std::string, TextureEntry>::iterator it = textureMap.begin();
       it != textureMap.end(); ++it) {
    delete it->second.texture;
  }
  textureMap.clear();
}

OkTextureHandler::~OkTextureHandler() {
  cleanup();
}
