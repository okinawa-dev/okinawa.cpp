#ifndef OK_INPUT_HPP
#define OK_INPUT_HPP

#include <GLFW/glfw3.h>

/**
 * @brief Input state structure to hold the current state of input.
 *        Contains boolean flags for movement and rotation controls.
 */
class OkInputState {
public:
  // Constructor
  OkInputState() {
    forward     = false;
    backward    = false;
    strafeLeft  = false;
    strafeRight = false;
    turnLeft    = false;
    turnRight   = false;
    turnUp      = false;
    turnDown    = false;
  }

  // Movement state
  bool forward;
  bool backward;
  bool strafeLeft;
  bool strafeRight;

  // Rotation state
  bool turnLeft;
  bool turnRight;
  bool turnUp;
  bool turnDown;
};

/**
 * @brief Input class to handle user input for the application.
 *        It processes input events and provides the current state of input.
 */
class OkInput {
public:
  // Static initialization
  static void initialize(GLFWwindow *window) {
    OkInput::window = window;
  }

  // Delete constructor, copy constructor and assignment
  OkInput()                           = delete;
  OkInput(const OkInput &)            = delete;
  OkInput &operator=(const OkInput &) = delete;

  static void         process();
  static OkInputState getState();

  // Constants
  static constexpr float MOVE_SPEED     = 5.0f;
  static constexpr float ROTATION_SPEED = 2.0f;

private:
  static GLFWwindow *window;
};

#endif
