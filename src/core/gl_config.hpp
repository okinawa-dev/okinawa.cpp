#ifndef OK_GL_CONFIG_HPP
#define OK_GL_CONFIG_HPP

// Silence deprecation warnings on macOS
#if (__APPLE__)
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#define GLFW_INCLUDE_GLCOREARB
// Next two defined to avoid using gl.h and gl3.h at the same time
// (which causes warnings)
#define __gl_h_
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

// the pragmas below are used to control include-what-you-use (iwyu) behavior
// and ensure that the necessary OpenGL headers are exported correctly
// and to avoid warnings about missing includes when using strict configuration

#include <GLFW/glfw3.h>      // IWYU pragma: export
#include <OpenGL/gltypes.h>  // IWYU pragma: export

#endif  // OK_GL_CONFIG_HPP
