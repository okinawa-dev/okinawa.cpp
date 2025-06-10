#include "texture.hpp"
#include "../core/gl_config.hpp"
#include "../utils/logger.hpp"
#include <string>

// Include stb_image for image loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

/**
 * @brief Constructor for the OkTexture class.
 *        Loads the texture from the specified path.
 * @param path The path to the texture file.
 */
OkTexture::OkTexture(const std::string &path) {
  this->path = path;
  loaded     = false;
  id         = 0;
  width      = 0;
  height     = 0;
  channels   = 0;

  OkLogger::info("Texture :: Loading texture: " + path);

  // Load image data
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 0);

  if (!data) {
    OkLogger::error("Texture :: Failed to load texture: " + path + " (" +
                    std::string(stbi_failure_reason()) + ")");
    return;
  }

  // Create OpenGL texture
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Load texture data
  GLenum format = channels == 4 ? GL_RGBA : GL_RGB;
  glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0,
               format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  // Free image data
  stbi_image_free(data);

  loaded = true;
}

/**
 * @brief Constructor for creating an empty texture with specified dimensions.
 * @param width Width of the texture.
 * @param height Height of the texture.
 * @param channels Number of color channels (3 for RGB, 4 for RGBA).
 */
OkTexture::OkTexture(int width, int height, int channels) {
  this->path     = "";  // No file path for raw textures
  this->width    = width;
  this->height   = height;
  this->channels = channels;
  this->loaded   = false;
  this->id       = 0;

  // Create OpenGL texture
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

/**
 * @brief Constructor for creating and initializing texture from raw data.
 * @param data Pointer to raw pixel data
 * @param width Width of the texture
 * @param height Height of the texture
 * @param channels Number of color channels (3 for RGB, 4 for RGBA)
 */
OkTexture::OkTexture(const unsigned char *data, int width, int height,
                     int channels) {
  this->path     = "";  // No file path for raw textures
  this->width    = width;
  this->height   = height;
  this->channels = channels;
  this->loaded   = false;
  this->id       = 0;

  // Create OpenGL texture
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Load the raw data
  GLenum format = channels == 4 ? GL_RGBA : GL_RGB;
  glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0,
               format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  loaded = true;
}

/**
 * @brief Destructor for the OkTexture class.
 *        Cleans up the texture resources.
 */
OkTexture::~OkTexture() {
  if (loaded) {
    glDeleteTextures(1, &id);
  }
}

/**
 * @brief Binds the texture for rendering.
 */
void OkTexture::bind() const {
  if (loaded) {
    glBindTexture(GL_TEXTURE_2D, id);
  }
}

/**
 * @brief Unbinds the currently bound texture.
 */
void OkTexture::unbind() {
  glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * @brief Creates a texture from raw data.
 * @param data Pointer to the raw image data.
 * @param width Width of the texture.
 * @param height Height of the texture.
 * @param format Format of the texture (GL_RGBA, GL_RGB, etc.).
 * @param internalFormat Internal format of the texture (default: GL_RGBA).
 * @return True if the texture was created successfully, false otherwise.
 */
bool OkTexture::createFromRawData(const unsigned char *data, int width,
                                  int height, GLenum format,
                                  GLenum internalFormat) {
  if (!data || width <= 0 || height <= 0) {
    return false;
  }

  this->width    = width;
  this->height   = height;
  this->channels = format == GL_RGBA ? 4 : 3;

  if (!loaded) {
    glGenTextures(1, &id);
  }

  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), width,
               height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  loaded = true;
  return true;
}
