#include "gl_config.hpp"

// Single home for the platform-specific OpenGL loader initialization. See the
// declaration in gl_config.hpp for the contract.
bool okInitGlLoader() {
#ifndef __APPLE__
  // On non-Apple platforms the OpenGL entry points come from GLEW; without
  // glewInit() they are null and the first GL call dereferences a null pointer.
  glewExperimental = GL_TRUE;
  return glewInit() == GLEW_OK;
#else
  // macOS links the OpenGL framework directly; nothing to load.
  return true;
#endif
}
