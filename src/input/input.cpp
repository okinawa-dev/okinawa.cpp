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
    OkLogger::error("Input :: Window is null");
  }

  _window        = window;
  _mouseCallback = callback;

  // Initialize states
  _currentState = OkInputState();
  _prevState    = OkInputState();

  // Initialize key arrays
  std::memset(_currentKeys, 0, sizeof(_currentKeys));
  std::memset(_prevKeys, 0, sizeof(_prevKeys));

  OkLogger::info("Setting mouse callback...");
  glfwSetCursorPosCallback(window, _mouseCallback);
  // Hide and capture cursor
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

  // Update current key states - convert from GLFW to OkKeys
  for (int i = 0; i < OK_KEY_COUNT; i++) {
    OkKey okKey   = static_cast<OkKey>(i);
    int   glfwKey = OkKeys::okKeyToGLFW(okKey);
    if (glfwKey != GLFW_KEY_UNKNOWN) {
      _currentKeys[i] = glfwGetKey(_window, glfwKey) == GLFW_PRESS;
    } else {
      _currentKeys[i] = false;
    }
  }

  // Update movement state (continuous press) - using OkKeys directly
  _currentState.forward     = isKeyHeld(OK_KEY_W);
  _currentState.backward    = isKeyHeld(OK_KEY_S);
  _currentState.strafeLeft  = isKeyHeld(OK_KEY_A);
  _currentState.strafeRight = isKeyHeld(OK_KEY_D);

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

  // Update exit state (just pressed)
  _currentState.exit = isKeyJustPressed(OK_KEY_ESCAPE);
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
