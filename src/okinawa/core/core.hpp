#ifndef OK_CORE_HPP
#define OK_CORE_HPP

#include "../handlers/scenes.hpp"
#include "../input/input.hpp"
#include "./camera.hpp"
#include "gl_config.hpp"
#include <functional>
#include <string>
#include <vector>

// Forward declaration: the MCP server type is only a pointer here, and its
// full definition (and dependencies) stays out of this public header.
class OkMcpServer;

// Forward declaration: the active avatar is tracked as a pointer (not owned).
class OkAvatar;

/**
 * @brief Core class for the Okinawa engine.
 *        It handles the initialization of OpenGL, shaders, and the main loop.
 */
class OkCore {
public:
  // Loopback port the in-engine MCP server listens on by default.
  static const int DEFAULT_MCP_PORT = 8765;

  // Callback type for engine loop
  using OkCoreCallback = std::function<void(float deltaTime)>;

  // Delete constructor to prevent instantiation
  OkCore() = delete;

  // Core initialization and loop
  static bool initialize();
  static void loop(const OkCoreCallback &stepCallback,
                   const OkCoreCallback &drawCallback);

  /**
   * @brief Draw over the finished frame, immediately before the swap.
   *
   *        The draw callback passed to loop() runs inside the frame:
   *        before the post-process composite, so whatever it draws is
   *        bloomed, fogged and depth-of-field blurred along with the
   *        world. That is right for anything belonging to the scene and
   *        wrong for anything belonging to the application on top of it,
   *        which wants to be read rather than looked through.
   *
   *        This one runs after the composite and after the interface
   *        pass, with the default framebuffer bound, so it paints on the
   *        finished image. Set it to an empty function to remove it.
   *
   * @param overlayCallback Called once per frame with the frame delta.
   */
  static void setOverlayCallback(const OkCoreCallback &overlayCallback);

  /**
   * @brief Run something once, when the loop is ending, before anything
   *        is torn down.
   *
   *        An application usually has one thing to do on the way out --
   *        write down where the user was, flush what they were editing --
   *        and nowhere safe to do it. After `loop()` returns is too late:
   *        it calls `exit()` before returning, which deletes the scene,
   *        the input and every camera, so an application asking where its
   *        camera was is asking a deleted object. Before `loop()` is too
   *        early, because the answer is not known yet.
   *
   *        This runs in between, once, on the loop thread, whatever ended
   *        the loop: the window's close button, `askForExit()`, or the
   *        MCP `quit` tool. It does not run when the process is killed
   *        from outside -- nothing can.
   *
   * @param exitCallback Called with the loop already stopped and the
   *                     scene still intact. Set an empty function to
   *                     remove it.
   */
  static void setExitCallback(const std::function<void()> &exitCallback);

  /**
   * @brief Give the window an icon of the application's own.
   *
   *        Pass the same picture at several sizes and the window system
   *        picks the one it wants; a 16 and a 32 is enough for a title
   *        bar and a task bar, and drawing each size separately beats
   *        handing over one large one for it to shrink.
   *
   *        **macOS puts it on the Dock tile instead.** Its windows carry
   *        no icon at all, so there is nothing for the window system to
   *        take; the application's icon on that platform is the tile,
   *        and AppKit sets it (see `core/mac_icon.mm`). A bundle says
   *        the same thing statically, through `Contents/Resources` and
   *        `CFBundleIconFile`, and that is what the Finder and the
   *        application switcher read -- this is what a binary run
   *        straight out of a build directory needs, having no bundle to
   *        be read from. Either way the caller asks once and needs no
   *        #ifdef.
   *
   * @param pngPaths Square RGBA PNGs, any number, any sizes.
   * @return false when none of them could be read.
   */
  static bool setWindowIcon(const std::vector<std::string> &pngPaths);
  static void askForExit();
  static void exit();

  // Scene handler
  static OkSceneHandler *getSceneHandler() {
    return _sceneHandler;
  }

  // Getters
  static OkCamera *getCamera() {
    return _cameras[_currentCamera];
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

  // Camera management
  static void addCamera(OkCamera *camera);
  static void clearCameras();  // delete all cameras (install your own set)
  static void switchCamera(int index);
  static int  getCurrentCameraIndex() {
    return _currentCamera;
  }
  static int getCameraCount() {
    return static_cast<int>(_cameras.size());
  }
  // Camera at an index (nullptr out of range) and lookup by the name the
  // camera was created with (-1 when not found). Cameras are identified by
  // NAME across the engine surface (MCP, tools): indices are an internal
  // detail and the number-key switching is a temporary debug aid.
  static OkCamera *getCameraAt(int index) {
    return (index >= 0 && index < static_cast<int>(_cameras.size()))
               ? _cameras[index]
               : nullptr;
  }
  static int findCamera(const std::string &name);

  // Active avatar: the controlled entity that receives input each frame. Not
  // owned by the core (the game keeps ownership). Set null to fall back to the
  // free-fly camera control. Swapping it (on foot -> car) changes the controls
  // and camera behaviour in one call.
  static void setActiveAvatar(OkAvatar *avatar) {
    _activeAvatar = avatar;
  }
  static OkAvatar *getActiveAvatar() {
    return _activeAvatar;
  }

  // Apply a look delta in degrees: orbits the active avatar's current view if
  // it consumes the mouse, otherwise rotates the current camera (free-fly).
  static void applyLook(float yawDeg, float pitchDeg);

  // Apply a zoom delta (mouse-wheel notches; + zooms in, - out) to the current
  // camera (third-person distance / top-down height). Used by the physical
  // scroll wheel and the MCP zoom tool; works with physical input disabled.
  static void applyZoom(float delta);

  // The avatar orbit camera (third-person): the one the MCP `view` tool drives.
  // getOrbitCamera returns it without changing the active camera; activate also
  // makes it the rendered one. Null if the rig has no orbit camera.
  static OkCamera *getOrbitCamera();
  static OkCamera *activateOrbitCamera();

  // Enable the in-engine MCP server so an external agent can connect over
  // local HTTP and drive the app (v1: capture the rendered frame). Binds
  // 127.0.0.1:port. This symbol always exists; if the engine was built
  // without MCP support (OKINAWA_WITH_MCP undefined) it logs a warning and
  // does nothing, so apps compile identically with or without the server.
  static void enableMcpServer(int port = DEFAULT_MCP_PORT);

  // Ignore the user's physical keyboard/mouse input (e.g. when an instance is
  // meant to be driven only through the MCP server). Injected/MCP input still
  // works. Call after initialize().
  static void setIgnoreUserInput(bool ignore);

  // Ignore physical input for a while and then give it back on its own,
  // so an agent can take the keyboard for a measurement without a
  // relaunch -- and without being able to leave its owner locked out.
  // The block also lifts on ctrl + shift + escape. See OkInput.
  // @param seconds how long, clamped; zero or less blocks with no
  //        deadline, which is what the launch flag does.
  static void blockUserInput(double seconds);

  // Seconds until physical input returns: 0 when it is not blocked, and
  // a negative number for a block with no deadline.
  static double userInputBlockedFor();

private:
  static bool initializeOpenGL(int width, int height);
  static bool initializeShaders();

  static GLFWwindow             *_window;
  static std::vector<OkCamera *> _cameras;
  static int                     _currentCamera;
  static OkSceneHandler         *_sceneHandler;
  static GLuint                  _shaderProgram;
  static OkInput                *_input;
  static OkMcpServer            *_mcpServer;
  static OkAvatar               *_activeAvatar;
  static OkCoreCallback          _overlayCallback;
  static std::function<void()>   _exitCallback;

  static void mouseCallback(GLFWwindow *window, double xpos, double ypos);
  // Mouse-wheel scroll -> zoom the current camera (yoffset = notches).
  static void scrollCallback(GLFWwindow *window, double xoffset,
                             double yoffset);
  // Window focus changed -> release the captured cursor (see OkInput).
  static void focusCallback(GLFWwindow *window, int focused);
  // Mouse button -> click in the render area captures the cursor (pointer
  // lock).
  static void mouseButtonCallback(GLFWwindow *window, int button, int action,
                                  int mods);
  // Keep the GL viewport matching the (possibly HiDPI) framebuffer size.
  static void framebufferSizeCallback(GLFWwindow *window, int width,
                                      int height);
};

#endif
