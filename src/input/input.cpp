#include "input.hpp"

// Define the static member, static class members need to be defined outside the
// class
GLFWwindow *OkInput::window = nullptr;

/**
 * @brief Method to process current input events.
 */
void OkInput::process() {
  if (!window)
    return;

  // ESC KEY
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

/**
 * @brief Method to get the current state of input.
 * @return OkInputState object containing the current input state.
 */
OkInputState OkInput::getState() {
  OkInputState state;

  if (!window)
    return state;

  // Movement
  state.forward     = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
  state.backward    = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
  state.strafeLeft  = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
  state.strafeRight = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

  // Rotation
  state.turnLeft  = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
  state.turnRight = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
  state.turnUp    = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
  state.turnDown  = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;

  return state;
}
