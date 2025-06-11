#include "core.hpp"
#include "../config/config.hpp"
#include "../input/input.hpp"
#include "../shaders/shaders.hpp"
#include "../utils/assets.hpp"
#include "../utils/logger.hpp"
#include "core/camera.hpp"
#include "gl_config.hpp"
#include "handlers/scenes.hpp"
#include "math/rotation.hpp"
#include "scene/scene.hpp"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <string>
#include <vector>

// Static member initialization
GLFWwindow             *OkCore::_window = nullptr;
std::vector<OkCamera *> OkCore::_cameras;
int                     OkCore::_currentCamera = 0;
OkSceneHandler         *OkCore::_sceneHandler  = nullptr;
GLuint                  OkCore::_shaderProgram = 0;
OkInput                *OkCore::_input         = nullptr;

/**
 * @brief Initialize the core engine.
 *        This method sets up the OpenGL context, initializes shaders,
 *        and prepares the camera and scene handler.
 * @return True if initialization was successful, false otherwise.
 */
bool OkCore::initialize() {
  OkLogger::info("Core :: Initializing engine...");

  // Initialize asset management system first
  if (!OkAssets::initialize()) {
    OkLogger::error("Core :: Failed to initialize asset system");
    return false;
  }

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

  // Initialize default camera
  _cameras.push_back(new OkCamera("Default Camera", width, height));

  // Initialize input system
  _input = new OkInput(_window, &OkCore::mouseCallback);

  OkLogger::info("Core :: Engine initialized successfully");
  return true;
}

/**
 * @brief Mark the window for closing. In the next main loop iteration,
 *        the window will be closed and the engine will exit.
 */
void OkCore::askForExit() {
  glfwSetWindowShouldClose(_window, true);
}

/**
 * @brief Exit the engine and clean up resources.
 *        This method deletes the scene handler, input handler,
 *        and all cameras, and terminates GLFW.
 */
void OkCore::exit() {
  OkLogger::info("Core :: Exiting engine...");

  // Delete scene and input handlers first
  delete _sceneHandler;
  _sceneHandler = nullptr;

  delete _input;
  _input = nullptr;

  // Delete all cameras
  for (int i = 0; i < _cameras.size(); i++) {
    delete _cameras[i];
  }
  _cameras.clear();

  // Make sure we clean up OpenGL resources before destroying window
  if (_shaderProgram != 0) {
    glDeleteProgram(_shaderProgram);
    _shaderProgram = 0;
  }

  // Release OpenGL context before destroying window
  if (_window != nullptr) {
    glfwMakeContextCurrent(nullptr);
    glfwSetWindowShouldClose(_window, GLFW_TRUE);
    glfwDestroyWindow(_window);
    _window = nullptr;
  }

  // Finally terminate GLFW
  glfwTerminate();

  OkLogger::info("Core :: Engine exited successfully");
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

  _window = glfwCreateWindow(width, height, "WADViewer", nullptr, nullptr);
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
      OkAssets::loadShaderSource("fragmentshader.frag.glsl");
  std::string vertexShaderSource =
      OkAssets::loadShaderSource("vertexshader.vert.glsl");

  if (fragmentShaderSource.empty() || vertexShaderSource.empty()) {
    OkLogger::error("Core :: Failed to load shader source files");
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
void OkCore::loop(const OkCoreCallback &stepCallback,
                  const OkCoreCallback &drawCallback) {
  if (!_window || _cameras.empty()) {
    OkLogger::error("Core :: Cannot start loop without window or camera");
    return;
  }

  double lastFrameTime = glfwGetTime() * 1000.0;
  float  timePerFrame  = OkConfig::getFloat("graphics.time-per-frame");

  while (!glfwWindowShouldClose(_window)) {
    double currentTime = glfwGetTime() * 1000.0;
    double deltaTime   = currentTime - lastFrameTime;

    if (deltaTime >= timePerFrame) {
      lastFrameTime = currentTime;
      float dt      = (float)deltaTime;

      // Process input
      _input->process();

      // Handle camera switching based on input state
      OkInputState state = _input->getState();
      if (state.changeCamera != -1) {
        switchCamera(state.changeCamera);
      }

      // User step callback first to process input
      if (stepCallback) {
        stepCallback(dt);
      }

      // Call step function for the current camera
      _cameras[_currentCamera]->step(dt);

      OkScene *currentScene = _sceneHandler->getCurrentScene();

      // Update current scene
      if (currentScene) {
        currentScene->step(dt);
      }

      // Render
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      glUseProgram(_shaderProgram);

      // Set view and projection matrices
      GLint viewLoc = glGetUniformLocation(_shaderProgram, "view");
      GLint projLoc = glGetUniformLocation(_shaderProgram, "projection");

      // Use the current camera for view and projection
      glUniformMatrix4fv(viewLoc, 1, GL_FALSE,
                         _cameras[_currentCamera]->getViewPtr());
      glUniformMatrix4fv(projLoc, 1, GL_FALSE,
                         _cameras[_currentCamera]->getProjectionPtr());

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

      // Draw cameras (both for debugging and to render elements attached to
      // cameras, like interfaces)
      for (int i = 0; i < _cameras.size(); ++i) {
        _cameras[i]->draw();
      }

      glfwSwapBuffers(_window);
      glfwPollEvents();
    }
  }

  exit();
}

/**
 * @brief Mouse callback function for handling mouse movement.
 *        This function updates the camera direction based on mouse movement.
 * @param window The GLFW window that received the event.
 * @param xpos   The x-coordinate of the mouse cursor.
 * @param ypos   The y-coordinate of the mouse cursor.
 */
void OkCore::mouseCallback(GLFWwindow *window, double xpos, double ypos) {
  static float lastX      = static_cast<float>(xpos);
  static float lastY      = static_cast<float>(ypos);
  static bool  firstMouse = true;

  if (firstMouse) {
    lastX      = static_cast<float>(xpos);
    lastY      = static_cast<float>(ypos);
    firstMouse = false;
    return;
  }

  float xoffset = static_cast<float>(xpos) - lastX;
  // Reversed since y-coordinates range from bottom to top
  float yoffset = lastY - static_cast<float>(ypos);
  lastX         = static_cast<float>(xpos);
  lastY         = static_cast<float>(ypos);

  const float sensitivity = 0.05f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  // Get current rotation from camera
  OkRotation currentRotation = _cameras[_currentCamera]->getRotation();
  float      pitch           = currentRotation.getPitch();
  float      yaw             = currentRotation.getYaw();

  // Update angles
  // Convert to radians since OkRotation works in radians
  pitch += glm::radians(yoffset);
  yaw += glm::radians(-xoffset);

  // Constrain pitch to avoid flipping (in radians)
  const float maxPitch = glm::radians(89.0f);
  pitch                = std::min(pitch, maxPitch);
  pitch                = std::max(pitch, -maxPitch);

  // Set new rotation
  _cameras[_currentCamera]->setRotation(pitch, yaw, 0.0f);
}

/**
 * @brief Add a camera to the engine.
 * @param camera The camera to add.
 */
void OkCore::addCamera(OkCamera *camera) {
  _cameras.push_back(camera);
}

/**
 * @brief Switch to a different camera.
 * @param index The index of the camera to switch to.
 */
void OkCore::switchCamera(int index) {
  if (index >= 0 && index < static_cast<int>(_cameras.size())) {
    _currentCamera = index;
  }
}
