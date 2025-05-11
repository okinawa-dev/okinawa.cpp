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

  // Get an existing texture by name (returns nullptr if not found)
  OkTexture *getTexture(const std::string &name);

  std::vector<std::string> getTextureNames() const;

  // Create and store a texture from file
  OkTexture *createTextureFromFile(const std::string &path);

  // Create and store a texture from raw data
  OkTexture *createTextureFromRawData(const std::string   &name,
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
