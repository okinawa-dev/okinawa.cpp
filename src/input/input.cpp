#include "input.hpp"
#include "../utils/logger.hpp"
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

  // Update current key states
  for (int i = 0; i < GLFW_KEY_LAST; i++) {
    _currentKeys[i] = glfwGetKey(_window, i) == GLFW_PRESS;
  }

  // Update movement state (continuous press)
  _currentState.forward     = _currentKeys[GLFW_KEY_W];
  _currentState.backward    = _currentKeys[GLFW_KEY_S];
  _currentState.strafeLeft  = _currentKeys[GLFW_KEY_A];
  _currentState.strafeRight = _currentKeys[GLFW_KEY_D];

  // Update rotation state (continuous press)
  _currentState.turnLeft  = _currentKeys[GLFW_KEY_LEFT];
  _currentState.turnRight = _currentKeys[GLFW_KEY_RIGHT];
  _currentState.turnUp    = _currentKeys[GLFW_KEY_UP];
  _currentState.turnDown  = _currentKeys[GLFW_KEY_DOWN];

  // Update camera selection
  _currentState.changeCamera = -1;
  for (int i = 0; i < 9; i++) {
    if (_currentKeys[GLFW_KEY_1 + i]) {
      _currentState.changeCamera = i;
      break;
    }
  }

  // Update action states (just pressed)
  _currentState.action1 = isKeyJustPressed(GLFW_KEY_SPACE);
  _currentState.action2 = isKeyJustPressed(GLFW_KEY_T);
  _currentState.action3 = isKeyJustPressed(GLFW_KEY_R);
  _currentState.action4 = isKeyJustPressed(GLFW_KEY_F);

  // Update exit state (just pressed)
  _currentState.exit = isKeyJustPressed(GLFW_KEY_ESCAPE);
}

/**
 * @brief Method to check if a key was just pressed.
 * @param key The key to check.
 * @return True if the key was just pressed, false otherwise.
 */
bool OkInput::isKeyJustPressed(int key) const {
  return _currentKeys[key] && !_prevKeys[key];
}

/**
 * @brief Method to check if a key is being held down.
 * @param key The key to check.
 * @return True if the key is being held down, false otherwise.
 */
bool OkInput::isKeyHeld(int key) const {
  return _currentKeys[key];
}

/**
 * @brief Method to check if a key was just released.
 * @param key The key to check.
 * @return True if the key was just released, false otherwise.
 */
bool OkInput::isKeyJustReleased(int key) const {
  return !_currentKeys[key] && _prevKeys[key];
}

/**
 * @brief Method to get the current state of input.
 * @return OkInputState object containing the current input state.
 */
OkInputState OkInput::getState() const {
  return _currentState;
}
