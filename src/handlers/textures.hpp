#ifndef OK_TEXTURES_HPP
#define OK_TEXTURES_HPP

#include "../item/texture.hpp"
#include <map>
#include <string>

class OkTextureHandler {
private:
  // Map of texture name/path to texture and its reference count
  struct TextureEntry {
    OkTexture *texture;
    int        refCount;
  };

  std::map<std::string, TextureEntry> textureMap;

  // Private constructor - singleton
  OkTextureHandler();

  // Singleton instance
  static OkTextureHandler *instance;

public:
  // Delete copy constructor and assignment
  OkTextureHandler(const OkTextureHandler &)            = delete;
  OkTextureHandler &operator=(const OkTextureHandler &) = delete;

  // Get singleton instance
  static OkTextureHandler *getInstance();

  // Get or create texture from file
  OkTexture *getTexture(const std::string &path);

  // Get or create texture from raw data
  OkTexture *getTextureFromRawData(const std::string   &name,
                                   const unsigned char *data, int width,
                                   int height, int channels);

  // Reference counting
  void addReference(const std::string &name);
  void removeReference(const std::string &name);

  // Cleanup
  void cleanup();
  ~OkTextureHandler();
};

#endif
