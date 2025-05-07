#ifndef OK_CORE_HPP
#define OK_CORE_HPP

#if (__APPLE__)
#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLCOREARB
#endif

#include "../handlers/scenes.hpp"
#include "../input/input.hpp"
#include "../scene/scene.hpp"
#include "./camera.hpp"
#include <GLFW/glfw3.h>
#include <functional>

/**
 * @brief Core class for the Okinawa engine.
 *        It handles the initialization of OpenGL, shaders, and the main loop.
 */
class OkCore {
public:
  // Callback type for engine loop
  using OkCoreCallback = std::function<void(float deltaTime)>;

  // Delete constructor to prevent instantiation
  OkCore() = delete;

  // Core initialization and loop
  static bool initialize();
  static void loop(OkCoreCallback stepCallback, OkCoreCallback drawCallback);

  // Scene handler
  static OkSceneHandler *getSceneHandler() {
    return _sceneHandler;
  }

  // Getters
  static OkCamera *getCamera() {
    return _camera;
  }
  static GLFWwindow *getWindow() {
    return _window;
  }
  static GLuint getShaderProgram() {
    return _shaderProgram;
  }
  static OkInput *getInput() {
    return _input;
  }

private:
  static bool initializeOpenGL(int width, int height);
  static bool initializeShaders();

  static GLFWwindow     *_window;
  static OkCamera       *_camera;
  static OkSceneHandler *_sceneHandler;
  static GLuint          _shaderProgram;
  static OkInput        *_input;

  static void mouseCallback(GLFWwindow *window, double xpos, double ypos);
};

#endif
