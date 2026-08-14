#ifndef OK_INPUT_HPP
#define OK_INPUT_HPP

#include "../core/gl_config.hpp"  // IWYU pragma: keep
#include "keys.hpp"
#include <array>
#include <string>

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
  // Vertical nudge (held): move the controlled object straight up/down
  // (altitude fix-ups, e.g. after teleporting below the terrain).
  bool moveUp   = false;
  bool moveDown = false;

  // Mouse pan delta for this frame, in raw pixels (cursor captured only).
  // Consumed by pan-style controllers; mouse-look uses its own path.
  float panX = 0.0f;
  float panY = 0.0f;

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
  bool isKeyJustPressed(OkKey key) const;
  // True while key is being held down
  bool isKeyHeld(OkKey key) const;
  // True only on the frame when key is released
  bool isKeyJustReleased(OkKey key) const;

  // Get complete input state (for compatibility)
  OkInputState getState() const;

  // Synthetic input: mark a key as held for the next durationSeconds, as if
  // it were physically pressed. Used to drive the app programmatically (e.g.
  // from the MCP server). The injected state is OR-ed into the polled state
  // in process(), so it behaves exactly like a real key, including the
  // edge-triggered actions. Call from the engine loop thread.
  void injectKey(OkKey key, double durationSeconds);

  // Enable/disable physical (keyboard/mouse) input. When disabled, process()
  // ignores glfwGetKey polling (injected keys still apply) and the cursor is
  // released (GLFW_CURSOR_NORMAL); useful to drive an instance only via the
  // MCP server without the user's input interfering.
  void setPhysicalInputEnabled(bool enabled);
  bool isPhysicalInputEnabled() const {
    return _physicalEnabled;
  }

  // Text capture (the console). While captured, isKeyJustPressed/Held/
  // JustReleased and getState() report NOTHING to the game -- typing in
  // the console cannot trigger gameplay keys. The console itself reads
  // through the Raw variants, which ignore the capture flag.
  void setTextCapture(bool captured) {
    _textCapture = captured;
  }
  bool isTextCaptured() const {
    return _textCapture;
  }
  bool isKeyJustPressedRaw(OkKey key) const;
  bool isKeyHeldRaw(OkKey key) const;

  // Printable characters typed since the last drainChars() call (fed by
  // the GLFW char callback; ASCII only). The console drains this buffer
  // every frame while open.
  void        onChar(unsigned int codepoint);
  std::string drainChars();

  // Pointer lock. The cursor starts NORMAL (free OS pointer); a click inside
  // the render area captures it (hidden + locked) for mouse-look; ESC or focus
  // loss release it. While released, the title bar / OS chrome work normally.
  void setCursorCaptured(bool captured);

  /**
   * @brief Whether a click inside the view takes the pointer.
   *
   *        On by default, which is what a game wants: click in, the
   *        cursor disappears and the mouse steers the view.
   *
   *        An application that is *pointed at* rather than steered wants
   *        it off. An editor is the case: its cursor has to stay on
   *        screen because clicking is how things are chosen, and a
   *        pointer that vanishes on the first click leaves the user
   *        aiming at a window they can no longer see into. Turning this
   *        off also turns mouse-look off, since that reads the locked
   *        pointer's motion; drive the camera from keys instead.
   */
  void setPointerLockOnClick(bool enabled) {
    _pointerLockOnClick = enabled;
    if (!enabled) {
      setCursorCaptured(false);
    }
  }

  bool isPointerLockOnClick() const {
    return _pointerLockOnClick;
  }
  bool isCursorCaptured() const {
    return _cursorCaptured;
  }
  // Capture on a left click in the render area (from the mouse-button
  // callback).
  void onMouseButton(int button, int action);
  // Release the cursor when the window loses focus (from the focus callback).
  void onWindowFocus(bool focused);

  // Accumulate a raw mouse delta (pixels) from the cursor-position callback;
  // process() folds the accumulated delta into the frame's state (panX/panY)
  // and clears it, so controllers see one per-frame pan delta.
  void addPanDelta(float dx, float dy);

  // Constants
  static constexpr float MOVE_SPEED     = 5.0f;
  static constexpr float ROTATION_SPEED = 2.0f;

private:
  GLFWwindow                    *_window;
  MouseCallback                  _mouseCallback;
  OkInputState                   _currentState;  // Current frame's input state
  OkInputState                   _prevState;     // Previous frame's input state
  std::array<bool, OK_KEY_COUNT> _currentKeys;   // Current key states
  std::array<bool, OK_KEY_COUNT> _prevKeys;      // Previous key states
  // Per-key "injected until" timestamps (glfwGetTime seconds). A key counts as
  // pressed while glfwGetTime() < _injectedUntil[key].
  std::array<double, OK_KEY_COUNT> _injectedUntil;
  // When false, physical keyboard/mouse input is ignored (MCP-only control).
  bool _physicalEnabled;
  // Pointer lock state: true while the cursor is captured for mouse-look.
  bool _cursorCaptured;
  // Whether a click may take the pointer at all (see the setter).
  bool _pointerLockOnClick;
  // Mouse pan delta accumulated since the last process() (raw pixels).
  float _pendingPanX;
  float _pendingPanY;
  // Text capture flag (console open) and pending typed characters.
  bool        _textCapture;
  std::string _pendingChars;
};

#endif
