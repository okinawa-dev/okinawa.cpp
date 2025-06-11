#include "shaders.hpp"
#include "../config/config.hpp"
#include "../core/gl_config.hpp"
#include "../utils/logger.hpp"
#include <string>
#include <vector>

/**
 * @brief Compile a shader from source code.
 * @param source The shader source code.
 * @param shaderType The type of shader (GL_VERTEX_SHADER or
 * GL_FRAGMENT_SHADER).
 * @param shaderName The name of the shader (for error messages).
 * @return The compiled shader ID, or 0 on failure.
 */
GLuint OkShader::compile(const std::string &source, GLenum shaderType,
                         const std::string &shaderName) {
  GLuint shader = glCreateShader(shaderType);

  // if source is empty, return 0
  if (source.empty()) {
    OkLogger::error("Shader", "Source code is empty for " + shaderName);
    return 0;
  }

  const char *sourcePtr = source.c_str();
  glShaderSource(shader, 1, &sourcePtr, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    GLint             infoLogSize = OkConfig::getInt("opengl.infolog.size");
    std::vector<char> infoLog(infoLogSize);
    glGetShaderInfoLog(shader, infoLogSize, nullptr, infoLog.data());
    OkLogger::error("Shader", "Compilation error in " + shaderName + ":\n" +
                                  std::string(infoLog.data()));
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

GLuint OkShader::createProgram(const std::string &vertexSource,
                               const std::string &fragmentSource) {
  // Compile shaders
  GLuint vertexShader = compile(vertexSource, GL_VERTEX_SHADER, "vertex");
  if (!vertexShader) {
    return 0;
  }

  GLuint fragmentShader =
      compile(fragmentSource, GL_FRAGMENT_SHADER, "fragment");
  if (!fragmentShader) {
    glDeleteShader(vertexShader);
    return 0;
  }

  // Create and link program
  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  // Check linking errors
  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);

  if (!success) {
    GLint             infoLogSize = OkConfig::getInt("opengl.infolog.size");
    std::vector<char> infoLog(infoLogSize);
    glGetProgramInfoLog(program, infoLogSize, nullptr, infoLog.data());
    OkLogger::error("Shader", "Linking error:\n" + std::string(infoLog.data()));
    glDeleteProgram(program);
    program = 0;
  }

  // Clean up shaders
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return program;
}
