#ifndef OK_TEXTURE_HPP
#define OK_TEXTURE_HPP

#include "../core/gl_config.hpp"
#include <string>

/**
 * @brief OkTexture class for loading and managing textures.
 */
class OkTexture {
private:
  std::string path;
  bool        loaded;
  GLuint      id;
  int         width;
  int         height;
  int         channels;

public:
  // Constructor loads the texture from file
  explicit OkTexture(const std::string &path);

  // Constructor for creating texture from raw data
  OkTexture(int width, int height, int channels);

  // Constructor for creating and initializing texture from raw data
  OkTexture(const unsigned char *data, int width, int height, int channels);

  // Destructor handles cleanup
  ~OkTexture();

  // Delete copy constructor and assignment
  OkTexture(const OkTexture &)            = delete;
  OkTexture &operator=(const OkTexture &) = delete;

  // Texture operations
  void        bind() const;
  static void unbind();

  // Switch to nearest filtering (no mipmaps): crisp pixel textures such
  // as glyph atlases, where linear sampling would bleed between cells.
  void setNearestFiltering() const;

  // Replace the pixel data in place (same size and channel count the
  // texture was created with). RGBA/RGB raw data, no mipmap rebuild.
  void               updateRawData(const unsigned char *data, int newWidth,
                                   int newHeight) const;
  [[nodiscard]] bool isLoaded() const { return loaded; }

  // Getters
  [[nodiscard]] int                getWidth() const { return width; }
  [[nodiscard]] int                getHeight() const { return height; }
  [[nodiscard]] int                getChannels() const { return channels; }
  [[nodiscard]] const std::string &getPath() const { return path; }

  // Create texture from raw data
  bool createFromRawData(const unsigned char *data, int width, int height,
                         GLenum format, GLenum internalFormat = GL_RGBA);
};

#endif
