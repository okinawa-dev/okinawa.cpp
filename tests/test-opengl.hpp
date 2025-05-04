#if (__APPLE__)
#define GL_SILENCE_DEPRECATION
#endif

#ifndef TEST_OPENGL_HPP
#define TEST_OPENGL_HPP

#include "../src/config/config.hpp"
#include "../src/shaders/shaders.hpp"
#include <GLFW/glfw3.h>

/**
 * @brief Test GLFW context for OpenGL rendering.
 */
class TestGLFWContext {
public:
  TestGLFWContext() {
    if (!glfwInit()) {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    // Create a hidden window for OpenGL context
    glfwWindowHint(GLFW_SAMPLES, 4);  // 4x antialiasing
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);  // We want OpenGL 4.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(1, 1, "Test", nullptr, nullptr);
    if (!window) {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);
  }

  ~TestGLFWContext() {
    if (window) {
      glfwDestroyWindow(window);
    }
    glfwTerminate();
  }

private:
  GLFWwindow *window;
};

#endif  // TEST_OPENGL_HPP
