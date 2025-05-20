#ifndef OK_GL_CONFIG_HPP
#define OK_GL_CONFIG_HPP

// Silence deprecation warnings on macOS
#if (__APPLE__)
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#define GLFW_INCLUDE_GLCOREARB
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#endif  // OK_GL_CONFIG_HPP
