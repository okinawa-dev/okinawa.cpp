#ifndef OK_INPUT_HPP
#define OK_INPUT_HPP

#include "../core/gl_config.hpp"  // IWYU pragma: keep
#include "keys.hpp"
#include <array>
#include <string>
#include <vector>

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

  // Block physical input for a while, and give it back on its own.
  //
  // Same gate as setPhysicalInputEnabled, with a deadline on it. An agent
  // driving the app blocks the keyboard so a stray key cannot move the
  // view under a measurement -- and the thing that goes wrong is the
  // agent forgetting to give it back, leaving a person with a window
  // that ignores them. So the block expires by itself, and the combo in
  // the release combo lifts it whatever the agent asked for: nothing
  // running in the background may lock somebody out of their own window.
  //
  // @param seconds how long to hold it, clamped to
  //        [BLOCK_MIN_SECONDS, BLOCK_MAX_SECONDS]. Zero or less blocks
  //        with no deadline, which is what the launch flag does.
  void blockPhysicalInput(double seconds);

  // Seconds until physical input comes back: 0 when it is not blocked
  // -- or when it is off because the application asked rather than
  // because somebody outside is holding it -- and BLOCK_FOREVER when
  // the hold has no deadline.
  double physicalInputBlockedFor() const;

  // The shortest and longest a timed block may last, and the value
  // physicalInputBlockedFor() reports for one that never expires.
  static const double BLOCK_MIN_SECONDS;
  static const double BLOCK_MAX_SECONDS;
  static const double BLOCK_FOREVER;

  // How long a block lasts when the caller does not say. Long enough to
  // walk a viewpoint and capture it, short enough that a forgotten block
  // is an annoyance and not a lockout.
  static const double BLOCK_DEFAULT_SECONDS;

  // What a block would last for, given what was asked: the clamp, split
  // out so it can be tested without a window.
  static double clampBlockSeconds(double seconds);

  // CHORDS: several keys held at once, as one gesture.
  //
  // The modifiers, as bits, and what is held right now. Read from the
  // PHYSICAL state, so a combo can be recognised even while the game is
  // being kept from seeing input at all -- which is the case the engine
  // itself needs, to give the keyboard back.
  // constexpr and not const: a static const int with its value here has
  // no definition anywhere, so anything that takes it BY REFERENCE --
  // a test assertion, for one -- has nothing to link against.
  static constexpr int OK_MOD_SHIFT = 1;
  static constexpr int OK_MOD_CTRL  = 2;
  static constexpr int OK_MOD_ALT   = 4;
  static constexpr int OK_MOD_SUPER = 8;

  // THE CHORD THAT GIVES THE KEYBOARD BACK, and its name for whoever
  // has to tell somebody about it. One place, so the notice on screen
  // and the gesture that works cannot drift apart.
  //
  // Not escape: macOS does not deliver Escape to an application while
  // Control is held -- measured, with the modifiers arriving and the
  // Escape never doing so, while ctrl+shift+E arrived in the same
  // breath. A letter with no meaning of its own is the safe choice, and
  // the notice on screen is what makes it findable.
  static const int   RELEASE_MODS;
  static const OkKey RELEASE_KEY;
  static const char *releaseComboName();

  // Is this key down on the DEVICE, whatever the game is allowed to
  // see? For combos, and for saying so when one does not fire.
  bool isPhysicalKeyDown(OkKey key) const;

  // THE PRESS HISTORY: the last keys that went down on the device, in
  // order, with the modifiers that were held and when.
  //
  // A record and not a question, because the interesting question is
  // usually asked afterwards: whether a key ever arrived, whether a
  // combo was performed the way it was described, what somebody
  // actually pressed. Asked live, the answer depends on catching the
  // moment.
  //
  // It is also what a CONSECUTIVE combo is matched on -- one key after
  // another inside a time window, the way `okinawa.js` did it -- which
  // this engine does not have yet and which this makes possible.
  struct OkKeyPress {
    OkKey  key;
    int    mods;  // OK_MOD_* held when it went down
    double when;  // glfwGetTime seconds
  };
  static const int PRESS_HISTORY = 16;

  // The presses, oldest first, at most PRESS_HISTORY of them.
  const std::vector<OkKeyPress> &recentPresses() const {
    return _presses;
  }

  // A SIMULTANEOUS combo: did `key` go down THIS frame with exactly
  // `mods` already held?
  //
  // Exactly, not at least: ctrl+shift+escape must not fire on
  // ctrl+alt+shift+escape, or a combo would swallow every gesture that
  // contains it.
  bool isComboJustPressed(int mods, OkKey key) const;

  // Hide a key from the game until it is physically let go.
  //
  // This is what makes a combo usable rather than merely detectable. A
  // combo's keys mean something on their own -- escape quits most
  // applications -- so whoever acts on the combo has to take those keys
  // out of the frame, and keep them out until the gesture is over.
  // Without it, ctrl+shift+escape lifts an input block and the escape
  // still down closes the app a frame later, which is exactly what it
  // did.
  void consumeKeyUntilReleased(OkKey key);

  // The two above, as pure functions over a key state, so the matching
  // can be tested without a window. A CONSECUTIVE matcher would go
  // beside them, over `recentPresses` instead of over one frame.
  static int  modifiersOf(const std::array<bool, OK_KEY_COUNT> &keys);
  static bool comboJustPressed(const std::array<bool, OK_KEY_COUNT> &keys,
                               const std::array<bool, OK_KEY_COUNT> &prev,
                               int mods, OkKey key);

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
  std::array<bool, OK_KEY_COUNT> _currentKeys;   // What the game sees
  std::array<bool, OK_KEY_COUNT> _prevKeys;      // ...and saw last frame
  // The keyboard as the DEVICE reports it, whether or not the game is
  // allowed to see it. Combos are matched on this: the engine has to be
  // able to recognise the gesture that gives the keyboard back at the
  // very moment the keyboard is being ignored.
  std::array<bool, OK_KEY_COUNT> _physKeys;
  std::array<bool, OK_KEY_COUNT> _prevPhysKeys;
  // Keys taken out of the game's frame until they are let go: see
  // consumeKeyUntilReleased.
  std::array<bool, OK_KEY_COUNT> _consumed;
  // The last keys the device reported going down: see recentPresses.
  std::vector<OkKeyPress> _presses;
  // Per-key "injected until" timestamps (glfwGetTime seconds). A key counts as
  // pressed while glfwGetTime() < _injectedUntil[key].
  std::array<double, OK_KEY_COUNT> _injectedUntil;
  // When false, physical keyboard/mouse input is ignored (MCP-only control).
  bool _physicalEnabled;
  // When a timed block ends, on the same clock as glfwGetTime(). Zero
  // means the block (if any) has no deadline.
  double _blockUntil = 0.0;
  // Whether the current disable is a HOLD somebody asked for from
  // outside, or the application routing input elsewhere for a frame.
  // Only the first is reported and only the first is worth telling the
  // person about: an editor disables input whenever the cursor is over
  // a panel, and a notice saying an agent holds the keyboard every time
  // the mouse touches a window is a lie about what is happening.
  bool _blockIsHold = false;

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
