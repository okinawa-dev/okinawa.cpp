#ifndef OK_SHADERS_HPP
#define OK_SHADERS_HPP

#include <GLFW/glfw3.h>
#include <string>

class OkShader {
public:
  // Static class - no instantiation
  OkShader() = delete;

  // Compile shader from source
  static GLuint compile(const std::string &source, GLenum shaderType,
                        const std::string &shaderName);

  // Create shader program from vertex and fragment shaders
  static GLuint createProgram(const std::string &vertexSource,
                              const std::string &fragmentSource);

private:
};

#endif
