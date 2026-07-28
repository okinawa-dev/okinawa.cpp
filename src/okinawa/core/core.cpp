#include "core.hpp"
#include "../gui/console.hpp"
#include "../gui/gui.hpp"
#include "../avatar/avatar.hpp"
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
#include "../mcp/mcp-config.hpp"  // resolves OKINAWA_WITH_MCP (NDEBUG / force)
#ifdef OKINAWA_WITH_MCP
#include "../mcp/mcp-server.hpp"
#endif
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
OkMcpServer            *OkCore::_mcpServer     = nullptr;
OkAvatar               *OkCore::_activeAvatar  = nullptr;

/**
 * @brief Enable the in-engine MCP server so an external agent can connect
 *        over local HTTP and drive the app. See core.hpp for the contract.
 * @param port TCP port to bind on 127.0.0.1.
 */
void OkCore::enableMcpServer(int port) {
#ifdef OKINAWA_WITH_MCP
  if (_mcpServer != nullptr) {
    OkLogger::warning("MCP", "MCP server already enabled");
    return;
  }
  _mcpServer = new OkMcpServer(port);
  _mcpServer->start();
  OkLogger::info("MCP", "MCP server listening on http://127.0.0.1:" +
                            std::to_string(port) + "/mcp");
#else
  (void)port;
  OkLogger::warning(
      "MCP", "MCP server not compiled into this build (rebuild with --mcp=y)");
#endif
}

/**
 * @brief Initialize the core engine.
 *        This method sets up the OpenGL context, initializes shaders,
 *        and prepares the camera and scene handler.
 * @return True if initialization was successful, false otherwise.
 */
bool OkCore::initialize() {
  OkLogger::info("Core", "Initializing engine...");

  // Initialize asset management system first
  if (!OkAssets::initialize()) {
    OkLogger::error("Core", "Failed to initialize asset system");
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
    OkLogger::error("Core", "Failed to initialize shaders");
    return false;
  }

  // Initialize scene handler
  _sceneHandler = new OkSceneHandler();

  // Initialize the GUI pass (grid config, debug overlay) and the console.
  OkGui::initialize();
  OkConsole::initialize();

  // Typed characters feed the console while it is open.
  glfwSetCharCallback(_window, [](GLFWwindow * /*w*/, unsigned int cp) {
    if (_input != nullptr) {
      _input->onChar(cp);
    }
  });

  // Initialize default camera
  _cameras.push_back(new OkCamera("Default Camera", width, height));

  // Initialize input system
  _input = new OkInput(_window, &OkCore::mouseCallback);
  // Mouse-wheel zoom (camera distance / height); routed like the look callback.
  glfwSetScrollCallback(_window, &OkCore::scrollCallback);
  // Release the captured cursor when the window loses focus, so switching/moving
  // windows frees the OS pointer (recapture happens on the next click in-view).
  glfwSetWindowFocusCallback(_window, &OkCore::focusCallback);
  // Click inside the render area captures the cursor for mouse-look (pointer
  // lock); clicks on OS chrome (title bar) are not delivered here, so they work.
  glfwSetMouseButtonCallback(_window, &OkCore::mouseButtonCallback);

  OkLogger::info("Core", "Engine initialized successfully");
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
  OkLogger::info("Core", "Exiting engine...");

  // Destroy the GUI internal items while the GL context is still alive
  OkConsole::shutdown();
  OkGui::shutdown();

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

  OkLogger::info("Core", "Engine exited successfully");
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

  _window = glfwCreateWindow(width, height,
                             OkConfig::getString("window.title").c_str(),
                             nullptr, nullptr);
  if (!_window) {
    OkLogger::error("Core", "Failed to create GLFW window");
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(_window);

  // Initialize the OpenGL function loader (no-op on Apple, glewInit elsewhere).
  if (!okInitGlLoader()) {
    OkLogger::error("Core", "Failed to initialize the OpenGL loader");
    glfwTerminate();
    return false;
  }

  // Use the actual framebuffer size, not the logical window size, for the
  // viewport. On HiDPI / Retina displays the framebuffer is larger (e.g. 2x),
  // so using the window size renders the scene into only part of the window
  // (the lower-left quarter at 2x). Keep it in sync on DPI/size changes.
  int framebufferWidth  = 0;
  int framebufferHeight = 0;
  glfwGetFramebufferSize(_window, &framebufferWidth, &framebufferHeight);
  glViewport(0, 0, framebufferWidth, framebufferHeight);
  glfwSetFramebufferSizeCallback(_window, &OkCore::framebufferSizeCallback);

  return true;
}

/**
 * @brief Keep the OpenGL viewport in sync with the framebuffer size.
 * @param width  New framebuffer width in pixels.
 * @param height New framebuffer height in pixels.
 */
void OkCore::framebufferSizeCallback(GLFWwindow * /*window*/, int width,
                                     int height) {
  glViewport(0, 0, width, height);
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
    OkLogger::error("Core", "Failed to load shader source files");
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
    OkLogger::error("Core", "Cannot start loop without window or camera");
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

      // Console first: while open it captures the keyboard, so nothing
      // below (avatar, cameras, game callbacks) sees any key.
      OkConsole::update(dt);

      // Handle camera switching based on input state
      OkInputState state = _input->getState();
      if (state.changeCamera != -1) {
        switchCamera(state.changeCamera);
      }

      // Drive the active avatar from input (before the game's step callback so
      // it can react to the new pose). The controller carries its own reference
      // frame, so control is independent of which camera is rendered.
      if (_activeAvatar) {
        _activeAvatar->update(dt, state);
      }

      // User step callback first to process input
      if (stepCallback) {
        stepCallback(dt);
      }

      // Let the current camera reposition for what it observes (covers a
      // standalone spectator/fixed camera with no active avatar), then step it.
      if (!_cameras.empty()) {
        OkObject *target =
            _activeAvatar ? _activeAvatar->getControlledObject() : nullptr;
        _cameras[_currentCamera]->updateForTarget(target, dt);
      }
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
        OkLogger::error("Core", "Cannot find view/projection uniforms");
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

      // GUI pass: grid-placed OkItems over the frame, painter's order,
      // rendered with the calibrated GUI camera (see OkGui).
      OkGui::draw();

#ifdef OKINAWA_WITH_MCP
      // Run any queued MCP tool commands on this (GL) thread, after the frame
      // is rendered and before the buffers are swapped, so a capture reads the
      // freshly drawn back buffer.
      if (_mcpServer != nullptr) {
        _mcpServer->drainCommands();
      }
#endif

      glfwSwapBuffers(_window);
      glfwPollEvents();
    }
  }

  exit();
}

/**
 * @brief Ignore (or restore) the user's physical keyboard/mouse input.
 *        See core.hpp. MCP-injected input is unaffected.
 * @param ignore True to ignore physical input, false to restore it.
 */
void OkCore::setIgnoreUserInput(bool ignore) {
  if (_input != nullptr) {
    _input->setPhysicalInputEnabled(!ignore);
  }
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

  // Ignore mouse-look unless the cursor is captured (pointer lock): physical
  // input on, window focused, and the user has clicked into the view. So moving
  // the mouse in another app, or before clicking in, never rotates the view.
  // Re-baseline whenever not capturing so (re)capturing does not cause a jump.
  if ((_input != nullptr &&
       (!_input->isPhysicalInputEnabled() || !_input->isCursorCaptured())) ||
      (window != nullptr && glfwGetWindowAttrib(window, GLFW_FOCUSED) == 0)) {
    firstMouse = true;
    return;
  }

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

  // Raw pixel delta for pan-style controllers (folded per frame into the
  // input state). Cameras that consume look ignore pan and vice versa.
  if (_input != nullptr) {
    _input->addPanDelta(xoffset, yoffset);
  }

  const float sensitivity = 0.05f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  // Route the look delta to the current camera: it orbits (third-person),
  // free-flies (base/spectator) or ignores it (top-down/fixed), and repositions.
  applyLook(xoffset, yoffset);
}

/**
 * @brief Apply a look delta (in degrees) to the current camera. Each camera
 *        decides what to do (orbit for third-person, free-fly for the base/
 *        spectator, ignore for top-down/fixed). Repositions immediately so the
 *        resulting pose is up to date. Used by the MCP look tool, so it works
 *        even with physical input disabled.
 */
void OkCore::applyLook(float yawDeg, float pitchDeg) {
  if (_cameras.empty()) {
    return;
  }
  _cameras[_currentCamera]->look(yawDeg, pitchDeg);
  OkObject *target =
      _activeAvatar ? _activeAvatar->getControlledObject() : nullptr;
  _cameras[_currentCamera]->updateForTarget(target, 0.0f);
}

/**
 * @brief Apply a zoom delta to the current camera (third-person distance /
 *        top-down height; base camera ignores it) and reposition immediately.
 *        Used by the physical scroll wheel and the MCP zoom tool.
 */
void OkCore::applyZoom(float delta) {
  if (_cameras.empty()) {
    return;
  }
  _cameras[_currentCamera]->zoom(delta);
  OkObject *target =
      _activeAvatar ? _activeAvatar->getControlledObject() : nullptr;
  _cameras[_currentCamera]->updateForTarget(target, 0.0f);
}

OkCamera *OkCore::getOrbitCamera() {
  for (size_t i = 0; i < _cameras.size(); i++) {
    if (_cameras[i]->isOrbit()) {
      return _cameras[i];
    }
  }
  return nullptr;
}

OkCamera *OkCore::activateOrbitCamera() {
  for (size_t i = 0; i < _cameras.size(); i++) {
    if (_cameras[i]->isOrbit()) {
      _currentCamera = static_cast<int>(i);
      return _cameras[i];
    }
  }
  return _cameras.empty() ? nullptr : _cameras[_currentCamera];
}

/**
 * @brief Mouse-wheel callback: zoom the current camera. Ignored when physical
 *        input is disabled (--no-input) or the window is not focused, matching
 *        mouseCallback, so scrolling in another app does not zoom the view.
 */
void OkCore::scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
  (void)xoffset;
  if ((_input != nullptr && !_input->isPhysicalInputEnabled()) ||
      (window != nullptr && glfwGetWindowAttrib(window, GLFW_FOCUSED) == 0)) {
    return;
  }
  applyZoom(static_cast<float>(yoffset));
}

void OkCore::focusCallback(GLFWwindow *window, int focused) {
  (void)window;
  if (_input != nullptr) {
    _input->onWindowFocus(focused != 0);
  }
}

void OkCore::mouseButtonCallback(GLFWwindow *window, int button, int action,
                                 int mods) {
  (void)window;
  (void)mods;
  if (_input != nullptr) {
    _input->onMouseButton(button, action);
  }
}

/**
 * @brief Remove and delete all cameras (e.g. so a game can install its own set
 *        instead of the seeded default). Resets the current index.
 */
void OkCore::clearCameras() {
  for (std::size_t i = 0; i < _cameras.size(); i++) {
    delete _cameras[i];
  }
  _cameras.clear();
  _currentCamera = 0;
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
int OkCore::findCamera(const std::string &name) {
  for (size_t i = 0; i < _cameras.size(); i++) {
    if (_cameras[i]->getName() == name) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void OkCore::switchCamera(int index) {
  if (index >= 0 && index < static_cast<int>(_cameras.size())) {
    _currentCamera = index;
  }
}
