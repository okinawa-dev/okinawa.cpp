#include "core.hpp"
#include "../config/config.hpp"
#include "../input/input.hpp"
#include "../shaders/shaders.hpp"
#include "../utils/files.hpp"
#include "../utils/logger.hpp"
#include <glm/glm.hpp>
#include <string>

// Initialize static members
GLFWwindow     *OkCore::_window        = nullptr;
OkCamera       *OkCore::_camera        = nullptr;
OkSceneHandler *OkCore::_sceneHandler  = nullptr;
GLuint          OkCore::_shaderProgram = 0;
OkInput        *OkCore::_input         = nullptr;

/**
 * @brief Initialize the core engine.
 *        This method sets up the OpenGL context, initializes shaders,
 *        and prepares the camera and scene handler.
 * @return True if initialization was successful, false otherwise.
 */
bool OkCore::initialize() {
  OkLogger::info("Core :: Initializing engine...");

  // get width and height from config
  int width  = OkConfig::getInt("window.width");
  int height = OkConfig::getInt("window.height");

  // Initialize OpenGL context and window
  if (!initializeOpenGL(width, height)) {
    return false;
  }

  // Initialize shaders BEFORE scene setup
  if (!initializeShaders()) {
    OkLogger::error("Core :: Failed to initialize shaders");
    return false;
  }

  // Initialize scene handler
  _sceneHandler = new OkSceneHandler();

  // Initialize camera
  _camera = new OkCamera(width, height);

  // Initialize input system
  _input = new OkInput(_window, &OkCore::mouseCallback);

  OkLogger::info("Core :: Engine initialized successfully");
  return true;
}

/**
 * @brief Initialize OpenGL context and window.
 *        This method sets up the GLFW window and OpenGL context.
 * @param width  The width of the window.
 * @param height The height of the window.
 * @return True if initialization was successful, false otherwise.
 */
bool OkCore::initializeOpenGL(int width, int height) {
  glfwInit();
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  _window = glfwCreateWindow(width, height, "Heist", nullptr, nullptr);
  if (!_window) {
    OkLogger::error("Core :: Failed to create GLFW window");
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(_window);
  glViewport(0, 0, width, height);

  return true;
}

/**
 * @brief Initialize shaders for the engine.
 *        This method compiles and links the vertex and fragment shaders.
 * @return True if initialization was successful, false otherwise.
 */
bool OkCore::initializeShaders() {
  std::string fragmentShaderSource =
      OkFiles::readFile("./okinawa.cpp/src/shaders/fragmentshader.frag.glsl");
  std::string vertexShaderSource =
      OkFiles::readFile("./okinawa.cpp/src/shaders/vertexshader.vert.glsl");

  if (fragmentShaderSource.empty() || vertexShaderSource.empty()) {
    OkLogger::error("Core :: Failed to load shader files");
    return false;
  }

  GLuint vertexShader =
      OkShader::compile(vertexShaderSource, GL_VERTEX_SHADER, "vertex");
  GLuint fragmentShader =
      OkShader::compile(fragmentShaderSource, GL_FRAGMENT_SHADER, "fragment");

  if (!vertexShader || !fragmentShader) {
    return false;
  }

  _shaderProgram =
      OkShader::createProgram(vertexShaderSource, fragmentShaderSource);

  return _shaderProgram != 0;
}

/**
 * @brief Main loop of the engine.
 *        This method runs the main loop, processing input, updating the scene,
 *        and rendering the scene.
 * @param stepCallback Callback function for updating the scene.
 * @param drawCallback Callback function for rendering the scene.
 *        These callbacks are optional and can be used to add custom behavior
 *        during the main loop.
 * @note The loop will run until the window is closed.
 *       The step and draw callbacks are called every frame.
 */
void OkCore::loop(OkCoreCallback stepCallback, OkCoreCallback drawCallback) {
  double lastFrameTime = glfwGetTime() * 1000.0;
  float  timePerFrame  = OkConfig::getFloat("graphics.time-per-frame");

  while (!glfwWindowShouldClose(_window)) {
    double currentTime = glfwGetTime() * 1000.0;
    double deltaTime   = currentTime - lastFrameTime;

    if (deltaTime >= timePerFrame) {
      lastFrameTime = currentTime;
      float dt      = static_cast<float>(deltaTime);

      // Process input
      _input->process();

      OkScene *currentScene = _sceneHandler->getCurrentScene();

      // Update current scene
      if (currentScene) {
        currentScene->step(dt);
      }

      // User step callback
      if (stepCallback) {
        stepCallback(dt);
      }

      // Render
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      glUseProgram(_shaderProgram);

      // Set view and projection matrices
      GLint viewLoc = glGetUniformLocation(_shaderProgram, "view");
      GLint projLoc = glGetUniformLocation(_shaderProgram, "projection");
      glUniformMatrix4fv(viewLoc, 1, GL_FALSE, _camera->getViewPtr());
      glUniformMatrix4fv(projLoc, 1, GL_FALSE, _camera->getProjectionPtr());

      if (viewLoc == -1 || projLoc == -1) {
        OkLogger::error("Core :: Cannot find view/projection uniforms");
      }

      // Draw current scene
      if (currentScene) {
        currentScene->draw();
      }

      // User draw callback
      if (drawCallback) {
        drawCallback(dt);
      }

      glfwSwapBuffers(_window);
      glfwPollEvents();
    }
  }

  glDeleteProgram(_shaderProgram);
  glfwTerminate();
}

/**
 * @brief Mouse callback function for handling mouse movement.
 *        This function updates the camera direction based on mouse movement.
 * @param window The GLFW window that received the event.
 * @param xpos   The x-coordinate of the mouse cursor.
 * @param ypos   The y-coordinate of the mouse cursor.
 */
void OkCore::mouseCallback(GLFWwindow *window, double xpos, double ypos) {
  static float lastX      = xpos;
  static float lastY      = ypos;
  static bool  firstMouse = true;

  if (firstMouse) {
    lastX      = xpos;
    lastY      = ypos;
    firstMouse = false;
    return;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos;
  lastX         = xpos;
  lastY         = ypos;

  const float sensitivity = 0.1f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  // Get current angles from camera
  float yaw   = _camera->getYaw() + xoffset;
  float pitch = _camera->getPitch() + yoffset;

  // Constrain pitch
  if (pitch > 89.0f)
    pitch = 89.0f;
  if (pitch < -89.0f)
    pitch = -89.0f;

  // Calculate new direction vector
  OkPoint direction = OkMath::anglesToDirectionVector(pitch, yaw, 0.0f);
  _camera->setDirection(direction);
}
