#ifndef OK_TEXTURE_HPP
#define OK_TEXTURE_HPP

#if (__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

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
  // Constructor loads the texture
  explicit OkTexture(const std::string &path);
  // Destructor handles cleanup
  ~OkTexture();

  // Delete copy constructor and assignment
  OkTexture(const OkTexture &)            = delete;
  OkTexture &operator=(const OkTexture &) = delete;

  // Texture operations
  void        bind() const;
  static void unbind();
  bool        isLoaded() const {
    return loaded;
  }

  // Getters
  int getWidth() const {
    return width;
  }
  int getHeight() const {
    return height;
  }
  int getChannels() const {
    return channels;
  }
  const std::string &getPath() const {
    return path;
  }
};

#endif
