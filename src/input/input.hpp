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
    forward      = false;
    backward     = false;
    strafeLeft   = false;
    strafeRight  = false;
    turnLeft     = false;
    turnRight    = false;
    turnUp       = false;
    turnDown     = false;
    changeCamera = -1;
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

  // Camera selection (-1 if no camera key was pressed)
  int changeCamera;
};

/**
 * @brief Input class to handle user input for the application.
 *        It processes input events and provides the current state of input.
 */
class OkInput {
public:
  using MouseCallback = void (*)(GLFWwindow *, double, double);
  explicit OkInput(GLFWwindow *window, MouseCallback mouseCallback = nullptr);

  // No need for delete since we're not using static methods
  ~OkInput() = default;
  // Prevent copying
  OkInput(const OkInput &)            = delete;
  OkInput &operator=(const OkInput &) = delete;

  void         process();
  OkInputState getState();

  // Constants
  static constexpr float MOVE_SPEED     = 5.0f;
  static constexpr float ROTATION_SPEED = 2.0f;

private:
  GLFWwindow   *_window;
  MouseCallback _mouseCallback;
};

#endif
