#include "input.hpp"
#include "../utils/logger.hpp"

/**
 * @brief Constructor for OkInput class.
 */
OkInput::OkInput(GLFWwindow *window, MouseCallback callback) {
  if (!window) {
    OkLogger::error("Input :: Window is null");
  }

  _window        = window;
  _mouseCallback = callback;

  OkLogger::info("Setting mouse callback...");
  glfwSetCursorPosCallback(window, _mouseCallback);
  glfwSetInputMode(window, GLFW_CURSOR,
                   GLFW_CURSOR_DISABLED);  // Hide and capture cursor
}

/**
 * @brief Method to process current input events.
 */
void OkInput::process() {
  if (!_window)
    return;

  // ESC KEY
  if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(_window, GLFW_TRUE);
  }
}

/**
 * @brief Method to get the current state of input.
 * @return OkInputState object containing the current input state.
 */
OkInputState OkInput::getState() {
  OkInputState state;

  if (!_window)
    return state;

  // Movement
  state.forward     = glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS;
  state.backward    = glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS;
  state.strafeLeft  = glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS;
  state.strafeRight = glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS;

  // Rotation
  state.turnLeft  = glfwGetKey(_window, GLFW_KEY_LEFT) == GLFW_PRESS;
  state.turnRight = glfwGetKey(_window, GLFW_KEY_RIGHT) == GLFW_PRESS;
  state.turnUp    = glfwGetKey(_window, GLFW_KEY_UP) == GLFW_PRESS;
  state.turnDown  = glfwGetKey(_window, GLFW_KEY_DOWN) == GLFW_PRESS;

  return state;
}
