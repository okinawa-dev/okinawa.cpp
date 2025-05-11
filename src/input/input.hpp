#ifndef OK_INPUT_HPP
#define OK_INPUT_HPP

#include <GLFW/glfw3.h>

/**
 * @brief Input state structure to hold the current state of input.
 *        Contains boolean flags for movement and rotation controls.
 */
class OkInputState {
public:
  OkInputState()  = default;
  ~OkInputState() = default;

  // Movement state
  bool forward     = false;
  bool backward    = false;
  bool strafeLeft  = false;
  bool strafeRight = false;

  // Rotation state
  bool turnLeft  = false;
  bool turnRight = false;
  bool turnUp    = false;
  bool turnDown  = false;

  // Camera selection (-1 if no camera key was pressed)
  int changeCamera = -1;

  // Action buttons - will be true only on the frame when key is first pressed
  bool action1 = false;
  bool action2 = false;
  bool action3 = false;
  bool action4 = false;

  // Exit state
  bool exit = false;
};

/**
 * @brief Input class to handle user input for the application.
 *        It processes input events and provides the current state of input.
 */
class OkInput {
public:
  using MouseCallback = void (*)(GLFWwindow *, double, double);
  explicit OkInput(GLFWwindow *window, MouseCallback mouseCallback = nullptr);

  ~OkInput() = default;
  // Prevent copying
  OkInput(const OkInput &)            = delete;
  OkInput &operator=(const OkInput &) = delete;

  // Process input and update states
  void process();

  // Input state retrieval methods
  // True only on the frame when key is first pressed
  bool isKeyJustPressed(int key) const;
  // True while key is being held down
  bool isKeyHeld(int key) const;
  // True only on the frame when key is released
  bool isKeyJustReleased(int key) const;

  // Get complete input state (for compatibility)
  OkInputState getState() const;

  // Constants
  static constexpr float MOVE_SPEED     = 5.0f;
  static constexpr float ROTATION_SPEED = 2.0f;

private:
  GLFWwindow   *_window;
  MouseCallback _mouseCallback;
  OkInputState  _currentState;                // Current frame's input state
  OkInputState  _prevState;                   // Previous frame's input state
  bool          _currentKeys[GLFW_KEY_LAST];  // Current key states
  bool          _prevKeys[GLFW_KEY_LAST];     // Previous key states
};

#endif
