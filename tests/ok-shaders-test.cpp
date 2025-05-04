#if (__APPLE__)
// use this define to silence deprecation warning messages
#define GL_SILENCE_DEPRECATION
#endif

#include "../src/config/config.hpp"
#include "../src/shaders/shaders.hpp"
#include <catch2/catch_test_macros.hpp>

#include "test-opengl.hpp"

// Test data
const char *validVertexShader = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
        gl_Position = vec4(aPos, 1.0);
    }
)";

const char *invalidVertexShader = R"(
  #version 330 core
  layout (location = 0) in vec3 aPos;
  void main() {
      gl_Position = vec4(aPos);  // Missing required arguments
  }
)";

const char *validFragmentShader = R"(
  #version 330 core
  out vec4 FragColor;
  void main() {
      FragColor = vec4(1.0, 1.0, 1.0, 1.0);
  }
)";

const char *invalidFragmentShader = R"(
  #version 330 core
  out vec4 FragColor;
  void main() {
      FragColor = invalidFunction();  // Function doesn't exist
  }
)";

TEST_CASE("OkShader compilation", "[shaders]") {
  TestGLFWContext context;  // OpenGL context

  SECTION("Valid vertex shader compilation") {
    GLuint shaderId =
        OkShader::compile(validVertexShader, GL_VERTEX_SHADER, "test_vertex");
    // INFO("Shader compilation failed. Check shader source and OpenGL
    // context");
    REQUIRE(shaderId != 0);
    if (shaderId) {
      glDeleteShader(shaderId);
    }
  }

  SECTION("Invalid vertex shader compilation") {
    GLuint shaderId =
        OkShader::compile(invalidVertexShader, GL_VERTEX_SHADER, "test_vertex");
    REQUIRE(shaderId == 0);
  }

  SECTION("Empty shader source") {
    GLuint shaderId = OkShader::compile("", GL_VERTEX_SHADER, "empty_shader");
    REQUIRE(shaderId == 0);
  }
}

TEST_CASE("OkShader program creation", "[shaders]") {
  TestGLFWContext context;  // OpenGL context

  SECTION("Valid program creation") {
    GLuint program =
        OkShader::createProgram(validVertexShader, validFragmentShader);
    REQUIRE(program != 0);
    glDeleteProgram(program);
  }

  SECTION("Invalid vertex shader") {
    GLuint program =
        OkShader::createProgram(invalidVertexShader, validFragmentShader);
    REQUIRE(program == 0);
  }

  SECTION("Empty shaders") {
    GLuint program = OkShader::createProgram("", "");
    REQUIRE(program == 0);
  }
}

TEST_CASE("OkShader program creation error cases", "[shaders]") {
  TestGLFWContext context;

  SECTION("Program linking failure with invalid shaders") {
    const char *invalidVertexShader = R"(
        #version 330 core
        // Completely invalid GLSL
        this is not valid shader code
        void main() {
            nonsense code here
        }
    )";

    GLuint program =
        OkShader::createProgram(invalidVertexShader, validFragmentShader);
    REQUIRE(program == 0);
  }

  SECTION("Program linking failure with valid vertex shader and invalid "
          "fragment shader") {
    const char *validVertexShader     = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";
    const char *invalidFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = invalidFunction();  // Function doesn't exist
        }
    )";
    GLuint      program =
        OkShader::createProgram(validVertexShader, invalidFragmentShader);
    REQUIRE(program == 0);
  }

  SECTION("Program linking failure with mismatched interface") {
    const char *vertexWithoutOutput = R"(
          #version 330 core
          layout (location = 0) in vec3 aPos;
          void main() {
              gl_Position = vec4(aPos, 1.0);
              // No output variable
          }
      )";

    const char *fragmentWithInput = R"(
          #version 330 core
          in vec4 color;  // Input that doesn't exist in vertex shader
          out vec4 FragColor;
          void main() {
              FragColor = color;  // Using undefined input
          }
      )";

    GLuint program =
        OkShader::createProgram(vertexWithoutOutput, fragmentWithInput);
    REQUIRE(program == 0);
  }
}
