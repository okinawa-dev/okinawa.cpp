#include "core.hpp"
#include "../avatar/avatar.hpp"
#include "../config/config.hpp"
#include "../gui/console.hpp"
#include "../gui/gui.hpp"
#include "../gui/input_notice.hpp"
#include "../gui/stats.hpp"
#include "../input/input.hpp"
#include "../lighting/light_clusters.hpp"
#include "../lighting/lighting.hpp"
#include "../lighting/shadow_map.hpp"
#include "../lighting/skybox.hpp"
#include "../math/frustum.hpp"
#include "../mcp/mcp-config.hpp"  // resolves OKINAWA_WITH_MCP (NDEBUG / force)
#include "../render/postprocess.hpp"
#include "../shaders/shaders.hpp"
#include "../utils/assets.hpp"
#include "../utils/async_loader.hpp"
#include "../utils/logger.hpp"
#include "core/camera.hpp"
#include "gl_config.hpp"
#include "handlers/scenes.hpp"
#include "math/rotation.hpp"
#include "scene/scene.hpp"
#ifdef OKINAWA_WITH_MCP
#include "../mcp/mcp-server.hpp"
#endif
#include <algorithm>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <stb/stb_image.h>
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
std::function<void()>   OkCore::_exitCallback;
OkAvatar               *OkCore::_activeAvatar = nullptr;
OkCore::OkCoreCallback  OkCore::_overlayCallback;

/**
 * @brief Enable the in-engine MCP server so an external agent can connect
 *        over local HTTP and drive the app. See core.hpp for the contract.
 * @param port TCP port to bind on 127.0.0.1.
 */
namespace {

  // Milliseconds the main thread may spend per frame turning finished
  // background work into engine objects, before the rest waits a frame.
  const float DEFAULT_LOAD_BUDGET_MS = 3.0f;

  // glfwGetTime reports seconds; the loop works in milliseconds.
  const double MS_PER_SECOND = 1000.0;

}  // namespace

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
  OkLighting::initialize();
  OkPostProcess::initialize();
  OkLightClusters::initialize();
  OkShadowMap::initialize();
  OkGuiStats::initialize();
  OkAsyncLoader::initialize();
  // How long per frame the main thread may spend turning finished
  // background work into engine objects.
  OkConfig::setFloat("render.loadbudget", DEFAULT_LOAD_BUDGET_MS);
  // Beyond this range distance fog has swallowed the world, so drawing
  // is waste; 0 disables the cut. Projects tune it to their fog.
  OkConfig::setFloat("render.drawdistance", 0.0f);

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
  // Release the captured cursor when the window loses focus, so
  // switching/moving windows frees the OS pointer (recapture happens on the
  // next click in-view).
  glfwSetWindowFocusCallback(_window, &OkCore::focusCallback);
  // Click inside the render area captures the cursor for mouse-look (pointer
  // lock); clicks on OS chrome (title bar) are not delivered here, so they
  // work.
  glfwSetMouseButtonCallback(_window, &OkCore::mouseButtonCallback);

  OkLogger::info("Core", "Engine initialized successfully");
  return true;
}

/**
 * @brief Mark the window for closing. In the next main loop iteration,
 *        the window will be closed and the engine will exit.
 */
/**
 * @brief Install the overlay callback (see the header for what it is for).
 */
void OkCore::setOverlayCallback(const OkCoreCallback &overlayCallback) {
  _overlayCallback = overlayCallback;
}

#ifdef __APPLE__
// Defined in core/mac_icon.mm. Declared here rather than in a header
// because it has exactly one caller and saying so is the point.
bool okSetDockIcon(const unsigned char *rgba, int width, int height);
#endif

bool OkCore::setWindowIcon(const std::vector<std::string> &pngPaths) {
  if (_window == nullptr || pngPaths.empty()) {
    return false;
  }

  // Read them all first: glfwSetWindowIcon takes the whole set in one
  // call and copies what it needs, so the pixels only have to outlive
  // that call.
  std::vector<GLFWimage>                                        images;
  std::vector<std::unique_ptr<unsigned char, void (*)(void *)>> pixels;
  for (size_t i = 0; i < pngPaths.size(); i++) {
    int width    = 0;
    int height   = 0;
    int channels = 0;
    // Forced to four channels: the window system wants RGBA whatever
    // the file happens to carry.
    unsigned char *data =
        stbi_load(pngPaths[i].c_str(), &width, &height, &channels, 4);
    if (data == nullptr) {
      OkLogger::warning("Core", "Window icon not read: " + pngPaths[i]);
      continue;
    }
    GLFWimage image;
    image.width  = width;
    image.height = height;
    image.pixels = data;
    images.push_back(image);
    pixels.emplace_back(data, stbi_image_free);
  }
  if (images.empty()) {
    return false;
  }

  // Windows and Linux take the whole set and choose; macOS takes none
  // of them, because its windows have no icon. There the picture goes
  // on the Dock tile instead, which is what an application's icon means
  // on that platform -- and the largest one is the one worth sending,
  // since the tile is drawn large.
  glfwSetWindowIcon(_window, static_cast<int>(images.size()), images.data());

#ifdef __APPLE__
  size_t largest = 0;
  for (size_t i = 1; i < images.size(); i++) {
    if (images[i].width > images[largest].width) {
      largest = i;
    }
  }
  bool dock = okSetDockIcon(images[largest].pixels, images[largest].width,
                            images[largest].height);
  OkLogger::info(
      "Core", std::string("Window icon: ") +
                  (dock ? "dock tile set from " : "dock tile refused, from ") +
                  std::to_string(images[largest].width) + " px");
#else
  OkLogger::info("Core", "Window icon: " + std::to_string(images.size()) +
                             " size(s) given to the window system");
#endif

  return true;
}

/**
 * @brief Install the exit callback (see the header for what it is for).
 */
void OkCore::setExitCallback(const std::function<void()> &exitCallback) {
  _exitCallback = exitCallback;
}

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
  OkInputNotice::shutdown();
  OkGui::shutdown();
  OkSkybox::shutdown();
  OkPostProcess::shutdown();
  OkLightClusters::shutdown();
  OkShadowMap::shutdown();
  OkGuiStats::shutdown();
  OkAsyncLoader::shutdown();

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
  // Inside a macOS bundle GLFW moves the working directory to
  // `Contents/Resources` on its own, and it does it by default. That
  // undoes the one the asset search settled on, and every relative path
  // an application uses afterwards -- its data, its own assets, the
  // files it writes -- resolves inside the bundle instead of beside it.
  // The application decides where it works from; the window library
  // does not.
  glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
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

  // Vsync. With it on and double buffering, a frame that misses the
  // refresh by a hair waits for the whole next one, so the cost jumps
  // straight from one interval to two -- 60 fps to 30 with nothing in
  // between. That is correct for playing (no tearing, steady pacing)
  // and useless for measuring, because it hides what a change actually
  // cost: everything reads as 16.7 or 33.3 ms.
  //
  // `render.vsync` 0 asks for it to be off, and the frame times then
  // show the real work rather than the next multiple of the refresh
  // interval. Note that a compositor may enforce vsync regardless --
  // macOS does -- so on those platforms the request is a no-op and
  // frame times stay quantised whatever this says.
  glfwSwapInterval(OkConfig::getInt("render.vsync"));

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
      OkAssets::loadShaderSource("world.frag.glsl");
  std::string vertexShaderSource =
      OkAssets::loadShaderSource("world.vert.glsl");

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

  double lastFrameTime = glfwGetTime() * MS_PER_SECOND;
  float  timePerFrame  = OkConfig::getFloat("graphics.time-per-frame");

  while (!glfwWindowShouldClose(_window)) {
    double currentTime = glfwGetTime() * MS_PER_SECOND;
    double deltaTime   = currentTime - lastFrameTime;

    if (deltaTime >= timePerFrame) {
      lastFrameTime = currentTime;
      float dt      = static_cast<float>(deltaTime);

      // Process input
      _input->process();

      // Console first: while open it captures the keyboard, so nothing
      // below (avatar, cameras, game callbacks) sees any key.
      OkConsole::update(dt);
      // Whether the keyboard is being ignored, and how to take it back.
      OkInputNotice::update();

      // Advance the day cycle and refresh the atmosphere values.
      OkLighting::update(dt);
      OkGuiStats::update(dt);
      // Finished background work becomes engine objects here, on the
      // thread that owns the rendering context, within a budget.
      OkAsyncLoader::drain(OkConfig::getFloat("render.loadbudget"));

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

      // Shadows: the scene's depth as the light sees it, rendered before
      // anything else so the world pass can sample it. Built around the
      // avatar (or the camera when there is none).
      if (currentScene != nullptr) {
        OkPoint   focus = _cameras[_currentCamera]->getPosition();
        OkAvatar *av    = OkCore::getActiveAvatar();
        if (av != nullptr && av->getControlledObject() != nullptr) {
          focus = av->getControlledObject()->getPosition();
        }
        // The map is fitted to the camera's own volume, so it needs the
        // matrix that defines it.
        glm::mat4 camView =
            glm::make_mat4(_cameras[_currentCamera]->getViewPtr());
        glm::mat4 camProj =
            glm::make_mat4(_cameras[_currentCamera]->getProjectionPtr());
        glm::mat4 camViewProj = camProj * camView;
        OkShadowMap::render(currentScene, glm::value_ptr(camViewProj),
                            focus.x(), focus.y(), focus.z());
      }

      // Post-process: with render.post on, the whole world pass renders
      // into the offscreen target and end() composites it to the window
      // before the camera-attached and GUI passes (which stay sharp).
      {
        int fbw = 0;
        int fbh = 0;
        glfwGetFramebufferSize(_window, &fbw, &fbh);
        OkPostProcess::begin(fbw, fbh);
      }

      // Render. Until the skybox exists, the clear colour IS the fog
      // colour, so distance fades into the "sky" seamlessly.
      const float *fogClear = OkLighting::getFogColor();
      glClearColor(fogClear[0], fogClear[1], fogClear[2], 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      // Back-face culling for the world pass: every mesh is single-sided
      // with a consistent outward winding (most exporters, terrain, avatars).
      // The skybox and the GUI pass disable it locally.
      glEnable(GL_CULL_FACE);
      glUseProgram(_shaderProgram);

      // Set view and projection matrices first, then paint the sky dome
      // behind everything with NEUTRAL atmosphere uniforms (the gradient
      // already carries the cycle's colours).
      {
        GLint viewLocS = glGetUniformLocation(_shaderProgram, "view");
        GLint projLocS = glGetUniformLocation(_shaderProgram, "projection");
        glUniformMatrix4fv(viewLocS, 1, GL_FALSE,
                           _cameras[_currentCamera]->getViewPtr());
        glUniformMatrix4fv(projLocS, 1, GL_FALSE,
                           _cameras[_currentCamera]->getProjectionPtr());
        GLint tintLocS = glGetUniformLocation(_shaderProgram, "sceneTint");
        GLint fogLocS  = glGetUniformLocation(_shaderProgram, "fogDensity");
        GLint litLocS  = glGetUniformLocation(_shaderProgram, "lightingOn");
        if (tintLocS != -1) {
          glUniform3f(tintLocS, 1.0f, 1.0f, 1.0f);
        }
        if (fogLocS != -1) {
          glUniform1f(fogLocS, 0.0f);
        }
        if (litLocS != -1) {
          glUniform1f(litLocS, 0.0f);
        }
        OkPoint camPos = _cameras[_currentCamera]->getPosition();
        OkSkybox::draw(camPos.x(), camPos.y(), camPos.z());
      }

      // Atmosphere uniforms for the world pass (the GUI pass resets them).
      {
        const float *tint = OkLighting::getSceneTint();
        GLint tintLoc     = glGetUniformLocation(_shaderProgram, "sceneTint");
        GLint fogColLoc   = glGetUniformLocation(_shaderProgram, "fogColor");
        GLint fogDenLoc   = glGetUniformLocation(_shaderProgram, "fogDensity");
        if (tintLoc != -1) {
          glUniform3f(tintLoc, tint[0], tint[1], tint[2]);
        }
        if (fogColLoc != -1) {
          glUniform3f(fogColLoc, fogClear[0], fogClear[1], fogClear[2]);
        }
        if (fogDenLoc != -1) {
          glUniform1f(fogDenLoc, OkLighting::getFogDensity());
        }
        // Height fog needs where the eye is and how fast the air thins
        // with altitude.
        GLint fogHLoc   = glGetUniformLocation(_shaderProgram, "fogHeight");
        GLint fogBLoc   = glGetUniformLocation(_shaderProgram, "fogBaseY");
        GLint fogEyeLoc = glGetUniformLocation(_shaderProgram, "fogEyePos");
        if (fogHLoc != -1) {
          glUniform1f(fogHLoc, OkConfig::getFloat("lighting.fog.height"));
        }
        if (fogBLoc != -1) {
          glUniform1f(fogBLoc, OkConfig::getFloat("lighting.fog.base"));
        }
        if (fogEyeLoc != -1) {
          OkPoint eye = _cameras[_currentCamera]->getPosition();
          glUniform3f(fogEyeLoc, eye.x(), eye.y(), eye.z());
        }
        // Gouraud sun (L3): direction/colour from the day cycle, over a
        // flat ambient floor. The GUI pass resets lightingOn to 0.
        GLint litLoc    = glGetUniformLocation(_shaderProgram, "lightingOn");
        GLint sunDirLoc = glGetUniformLocation(_shaderProgram, "sunDirection");
        GLint sunColLoc = glGetUniformLocation(_shaderProgram, "sunColor");
        GLint ambLoc    = glGetUniformLocation(_shaderProgram, "ambientLight");
        const float *sunDir = OkLighting::getSunDirection();
        const float *sunCol = OkLighting::getSunColor();
        if (litLoc != -1) {
          glUniform1f(litLoc, 1.0f);
        }
        if (sunDirLoc != -1) {
          glUniform3f(sunDirLoc, sunDir[0], sunDir[1], sunDir[2]);
        }
        if (sunColLoc != -1) {
          glUniform3f(sunColLoc, sunCol[0], sunCol[1], sunCol[2]);
        }
        if (ambLoc != -1) {
          glUniform1f(ambLoc, OkLighting::getAmbientLight());
        }
        OkShadowMap::bind(_shaderProgram);
        GLint plvLoc = glGetUniformLocation(_shaderProgram, "pointLightLevel");
        if (plvLoc != -1) {
          glUniform1f(plvLoc, OkLighting::getPointLightLevel());
        }

        // Clustered forward: assign the registry's lights to the frame's
        // cluster grid and bind the buffers the shader reads.
        {
          OkCamera *cam   = _cameras[_currentCamera];
          glm::mat4 viewM = glm::make_mat4(cam->getViewPtr());
          glm::mat4 projM = glm::make_mat4(cam->getProjectionPtr());
          int       fbw   = 0;
          int       fbh   = 0;
          glfwGetFramebufferSize(_window, &fbw, &fbh);
          OkLightClusters::update(viewM, projM, cam->getNearPlane(),
                                  cam->getFarPlane());
          OkLightClusters::bind(_shaderProgram, fbw, fbh, cam->getNearPlane(),
                                cam->getFarPlane());
        }
      }

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

      // Frustum culling for the world pass: six planes from
      // projection * view; OkItem::drawSelf skips items whose bounding
      // sphere is fully outside. Cleared before the camera-attached and
      // GUI passes (their elements live outside the world frustum).
      static OkFrustum frameFrustum;
      {
        glm::mat4 viewM =
            glm::make_mat4(_cameras[_currentCamera]->getViewPtr());
        glm::mat4 projM =
            glm::make_mat4(_cameras[_currentCamera]->getProjectionPtr());
        frameFrustum.setFromMatrix(projM * viewM);
        OkFrustum::resetStats();
        OkPoint eye = _cameras[_currentCamera]->getPosition();
        OkFrustum::setViewer(eye.x(), eye.y(), eye.z(),
                             OkConfig::getFloat("render.drawdistance"));
        OkFrustum::setActive(&frameFrustum);
      }

      // CPU cost of the frame's draws, timed on its own. See
      // OkGuiStats::recordDraw for why frame time cannot stand in for
      // it wherever the platform enforces vsync.
      std::chrono::steady_clock::time_point drawT0 =
          std::chrono::steady_clock::now();

      // Draw current scene
      if (currentScene) {
        currentScene->draw();
      }

      // User draw callback
      if (drawCallback) {
        drawCallback(dt);
      }

      OkFrustum::setActive(nullptr);
      OkGuiStats::recordDraw(std::chrono::duration<float, std::milli>(
                                 std::chrono::steady_clock::now() - drawT0)
                                 .count());

      // Composite the offscreen frame to the window (no-op when
      // render.post is off).
      OkPostProcess::end(_cameras[_currentCamera]->getNearPlane(),
                         _cameras[_currentCamera]->getFarPlane(), dt);

      // Draw cameras (both for debugging and to render elements attached to
      // cameras, like interfaces)
      for (int i = 0; i < _cameras.size(); ++i) {
        _cameras[i]->draw();
      }

      // GUI pass: grid-placed OkItems over the frame, painter's order,
      // rendered with the calibrated GUI camera (see OkGui).
      OkGui::draw();

      // Anything the application paints over the finished frame. Last,
      // so it sits on top of the world and of the interface pass, and
      // before the MCP drain, so a capture sees it.
      if (_overlayCallback) {
        _overlayCallback(dt);
      }

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

  // The application's last chance: the loop has stopped and nothing has
  // been deleted yet. Whatever it wants to remember about this session,
  // it has to read now -- exit() below takes the cameras with it.
  if (_exitCallback) {
    _exitCallback();
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
 * @brief Ignore physical input for a while, then give it back by itself.
 * @param seconds how long, clamped by OkInput; zero or less has no deadline.
 */
void OkCore::blockUserInput(double seconds) {
  if (_input != nullptr) {
    _input->blockPhysicalInput(seconds);
  }
}

/**
 * @brief Seconds until physical input returns.
 * @return 0 when input is not blocked, negative when the block never
 *         expires on its own, otherwise what is left of it.
 */
double OkCore::userInputBlockedFor() {
  return _input != nullptr ? _input->physicalInputBlockedFor() : 0.0;
}

/**
 * @brief How long escape has been held down, in seconds; 0 while it is up.
 */
double OkCore::userInputReleaseHeldFor() {
  return _input != nullptr ? _input->releaseHeldFor() : 0.0;
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

  // Physical input off, or the window not focused: nothing here applies,
  // and the baseline is dropped so that coming back does not arrive as
  // one enormous delta.
  if ((_input != nullptr && !_input->isPhysicalInputEnabled()) ||
      (window != nullptr && glfwGetWindowAttrib(window, GLFW_FOCUSED) == 0)) {
    firstMouse = true;
    return;
  }

  // The pointer's motion is followed from here on whether or not the
  // cursor is captured, so that capturing never arrives as a jump -- and
  // so that an application which has refused capture altogether can
  // still be told where the mouse went. What each half of that motion is
  // allowed to do is decided below.

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
  //
  // Fed while the cursor is captured, as mouse-look is -- or whenever
  // the application has turned capture off entirely. A tool whose cursor
  // is the instrument (`setPointerLockOnClick(false)`) never captures,
  // so a delta that waited for capture would never arrive at all, and
  // every pan-style controller would sit still in exactly the kind of
  // application that most wants one. Applications that leave capture on
  // are unaffected: for them the condition is still "captured", so a
  // mouse crossing the window before any click moves nothing.
  if (_input != nullptr &&
      (_input->isCursorCaptured() || !_input->isPointerLockOnClick())) {
    _input->addPanDelta(xoffset, yoffset);
  }

  // Mouse-look, on the other hand, always needs the pointer captured: a
  // free cursor aiming at a menu would be swinging the view behind it.
  if (_input != nullptr && !_input->isCursorCaptured()) {
    return;
  }

  const float sensitivity = 0.05f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  // Route the look delta to the current camera: it orbits (third-person),
  // free-flies (base/spectator) or ignores it (top-down/fixed), and
  // repositions.
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
void OkCore::scrollCallback(GLFWwindow *window, double xoffset,
                            double yoffset) {
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
