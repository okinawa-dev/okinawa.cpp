#include "input.hpp"
#include "../core/gl_config.hpp"
#include "../utils/logger.hpp"
#include "keys.hpp"
#include <cstring>

/**
 * @brief Constructor for OkInput class.
 */
OkInput::OkInput(GLFWwindow *window, MouseCallback callback) {
  if (!window) {
    OkLogger::error("Input", "Window is null");
  }

  _window        = window;
  _mouseCallback = callback;

  // Initialize states
  _currentState = OkInputState();
  _prevState    = OkInputState();

  // Initialize key arrays
  std::memset(_currentKeys, 0, sizeof(_currentKeys));
  std::memset(_prevKeys, 0, sizeof(_prevKeys));
  std::memset(_injectedUntil, 0, sizeof(_injectedUntil));
  _physicalEnabled = true;
  _cursorCaptured  = false;
  _pendingPanX     = 0.0f;
  _pendingPanY     = 0.0f;

  OkLogger::info("Input", "Setting mouse callback...");
  glfwSetCursorPosCallback(window, _mouseCallback);
  // Pointer lock: start with a normal OS cursor (so the window can be moved /
  // OS chrome used); a click inside the render area captures it for mouse-look.
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

/**
 * @brief Method to process current input events.
 */
void OkInput::process() {
  if (!_window)
    return;

  // Store previous key states
  std::memcpy(_prevKeys, _currentKeys, sizeof(_currentKeys));
  _prevState = _currentState;

  // Update current key states - convert from GLFW to OkKeys, OR-ing in any
  // synthetic (injected) key that is still within its hold window so it
  // behaves exactly like a physically held key.
  double now = glfwGetTime();
  for (int i = 0; i < OK_KEY_COUNT; i++) {
    OkKey okKey    = static_cast<OkKey>(i);
    int   glfwKey  = OkKeys::okKeyToGLFW(okKey);
    bool  physical = _physicalEnabled && glfwKey != GLFW_KEY_UNKNOWN &&
                    glfwGetKey(_window, glfwKey) == GLFW_PRESS;
    bool injected   = now < _injectedUntil[i];
    _currentKeys[i] = physical || injected;
  }

  // Update movement state (continuous press) - using OkKeys directly
  _currentState.forward     = isKeyHeld(OK_KEY_W);
  _currentState.backward    = isKeyHeld(OK_KEY_S);
  _currentState.strafeLeft  = isKeyHeld(OK_KEY_A);
  _currentState.strafeRight = isKeyHeld(OK_KEY_D);
  _currentState.moveUp      = isKeyHeld(OK_KEY_E);
  _currentState.moveDown    = isKeyHeld(OK_KEY_Q);

  // Fold the mouse pan delta accumulated by the cursor callback into this
  // frame's state, then clear the accumulator.
  _currentState.panX = _pendingPanX;
  _currentState.panY = _pendingPanY;
  _pendingPanX       = 0.0f;
  _pendingPanY       = 0.0f;

  // Update rotation state (continuous press) - using OkKeys directly
  _currentState.turnLeft  = isKeyHeld(OK_KEY_LEFT);
  _currentState.turnRight = isKeyHeld(OK_KEY_RIGHT);
  _currentState.turnUp    = isKeyHeld(OK_KEY_UP);
  _currentState.turnDown  = isKeyHeld(OK_KEY_DOWN);

  // Update camera selection - using OkKeys directly
  _currentState.changeCamera = -1;
  for (int i = 0; i < 9; i++) {
    // Convert OK_KEY_1 + i to OkKey
    OkKey okKeyNumber = static_cast<OkKey>(OK_KEY_1 + i);
    if (okKeyNumber < OK_KEY_COUNT && _currentKeys[okKeyNumber]) {
      _currentState.changeCamera = i;
      break;
    }
  }

  // Update action states (just pressed) - using OkKeys directly
  _currentState.action1 = isKeyJustPressed(OK_KEY_SPACE);
  _currentState.action2 = isKeyJustPressed(OK_KEY_T);
  _currentState.action3 = isKeyJustPressed(OK_KEY_R);
  _currentState.action4 = isKeyJustPressed(OK_KEY_F);

  // ESC: when the cursor is captured (pointer lock), release it instead of
  // exiting -- browser style. ESC only requests exit when already released.
  if (isKeyJustPressed(OK_KEY_ESCAPE) && _cursorCaptured) {
    setCursorCaptured(false);
    _currentState.exit = false;
  } else {
    _currentState.exit = isKeyJustPressed(OK_KEY_ESCAPE);
  }
}

/**
 * @brief Method to check if a key was just pressed.
 * @param key The key to check.
 * @return True if the key was just pressed, false otherwise.
 */
bool OkInput::isKeyJustPressed(OkKey key) const {
  if (key == OK_KEY_UNKNOWN || key < 0 || key >= OK_KEY_COUNT) {
    return false;
  }
  return _currentKeys[key] && !_prevKeys[key];
}

/**
 * @brief Method to check if a key is being held down.
 * @param key The key to check.
 * @return True if the key is being held down, false otherwise.
 */
bool OkInput::isKeyHeld(OkKey key) const {
  if (key == OK_KEY_UNKNOWN || key < 0 || key >= OK_KEY_COUNT) {
    return false;
  }
  return _currentKeys[key];
}

/**
 * @brief Method to check if a key was just released.
 * @param key The key to check.
 * @return True if the key was just released, false otherwise.
 */
bool OkInput::isKeyJustReleased(OkKey key) const {
  if (key == OK_KEY_UNKNOWN || key < 0 || key >= OK_KEY_COUNT) {
    return false;
  }
  return !_currentKeys[key] && _prevKeys[key];
}

/**
 * @brief Method to get the current state of input.
 * @return OkInputState object containing the current input state.
 */
OkInputState OkInput::getState() const {
  return _currentState;
}

/**
 * @brief Mark a key as synthetically held for the next durationSeconds.
 * @param key             The key to inject.
 * @param durationSeconds How long the key should read as pressed.
 */
void OkInput::injectKey(OkKey key, double durationSeconds) {
  if (key <= OK_KEY_UNKNOWN || key >= OK_KEY_COUNT) {
    return;
  }
  double until = glfwGetTime() + durationSeconds;
  // Extend, never shorten, an existing injection window for this key.
  if (until > _injectedUntil[key]) {
    _injectedUntil[key] = until;
  }
}

/**
 * @brief Enable or disable physical keyboard/mouse input.
 * @param enabled True to use real input, false for MCP-only control.
 */
void OkInput::setPhysicalInputEnabled(bool enabled) {
  _physicalEnabled = enabled;
  // Pointer lock is opt-in via a click; disabling physical input just makes sure
  // the cursor is released. It is never auto-captured here.
  if (!enabled) {
    setCursorCaptured(false);
  }
}

void OkInput::setCursorCaptured(bool captured) {
  _cursorCaptured = captured;
  if (_window != nullptr) {
    // GLFW_CURSOR_DISABLED hides + locks the cursor for raw mouse-look;
    // GLFW_CURSOR_NORMAL frees the OS pointer (and its acceleration).
    glfwSetInputMode(_window, GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
  }
}

void OkInput::onMouseButton(int button, int action) {
  // A left click inside the render area captures the cursor (pointer lock).
  // GLFW only delivers button events for the content area, so clicks on the
  // title bar / OS chrome never reach here and keep working normally.
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS &&
      _physicalEnabled && !_cursorCaptured && _window != nullptr &&
      glfwGetWindowAttrib(_window, GLFW_FOCUSED) != 0) {
    setCursorCaptured(true);
  }
}

void OkInput::onWindowFocus(bool focused) {
  // Release the cursor whenever the window loses focus, so switching to another
  // app frees the OS pointer and its acceleration. Recapture is not automatic:
  // the user clicks back into the view to resume mouse-look.
  if (!focused) {
    setCursorCaptured(false);
  }
}

void OkInput::addPanDelta(float dx, float dy) {
  _pendingPanX += dx;
  _pendingPanY += dy;
}
