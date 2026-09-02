#include "input.hpp"
#include "../core/gl_config.hpp"
#include "../utils/logger.hpp"
#include "keys.hpp"
#include <algorithm>
#include <cstring>

/**
 * @brief Constructor for OkInput class.
 */
namespace {

  // Number keys 1..9 select a camera; there is no camera 0 and the row
  // runs out at 9.
  const int CAMERA_KEYS = 9;

  // Printable ASCII, the only range the text capture accepts: anything
  // below is a control code and 127 is delete.
  const unsigned int ASCII_FIRST_PRINTABLE = 32;
  const unsigned int ASCII_LAST_PRINTABLE  = 126;

}  // namespace

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
  _currentKeys.fill(false);
  _prevKeys.fill(false);
  _physKeys.fill(false);
  _prevPhysKeys.fill(false);
  _consumed.fill(false);
  _injectedUntil.fill(0.0);
  _physicalEnabled    = true;
  _textCapture        = false;
  _pendingChars       = "";
  _cursorCaptured     = false;
  _pointerLockOnClick = true;
  _pendingPanX        = 0.0f;
  _pendingPanY        = 0.0f;

  OkLogger::info("Input", "Setting mouse callback...");
  glfwSetCursorPosCallback(window, _mouseCallback);
  // Pointer lock: start with a normal OS cursor (so the window can be moved /
  // OS chrome used); a click inside the render area captures it for mouse-look.
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

/**
 * @brief Method to process current input events.
 */

/**
 * @brief Which modifiers a key state has held down.
 * @return the OK_MOD_* bits, so a combo can ask for exactly a set.
 */
int OkInput::modifiersOf(const std::array<bool, OK_KEY_COUNT> &keys) {
  int mods = 0;
  if (keys[OK_KEY_LEFT_SHIFT] || keys[OK_KEY_RIGHT_SHIFT]) {
    mods |= OK_MOD_SHIFT;
  }
  if (keys[OK_KEY_LEFT_CONTROL] || keys[OK_KEY_RIGHT_CONTROL]) {
    mods |= OK_MOD_CTRL;
  }
  if (keys[OK_KEY_LEFT_ALT] || keys[OK_KEY_RIGHT_ALT]) {
    mods |= OK_MOD_ALT;
  }
  if (keys[OK_KEY_LEFT_SUPER] || keys[OK_KEY_RIGHT_SUPER]) {
    mods |= OK_MOD_SUPER;
  }
  return mods;
}

/**
 * @brief Did `key` go down this frame with exactly `mods` held?
 *
 * The EDGE is on the key and not on the modifiers, which is how a
 * person performs a combo: the modifiers go down first and are already
 * held when the key arrives.
 */
bool OkInput::comboJustPressed(const std::array<bool, OK_KEY_COUNT> &keys,
                               const std::array<bool, OK_KEY_COUNT> &prev,
                               int mods, OkKey key) {
  if (key == OK_KEY_UNKNOWN || key < 0 || key >= OK_KEY_COUNT) {
    return false;
  }
  if (!keys[key] || prev[key]) {
    return false;
  }
  return modifiersOf(keys) == mods;
}

/**
 * @brief Is this key down on the device, gate or no gate?
 */
bool OkInput::isPhysicalKeyDown(OkKey key) const {
  if (key == OK_KEY_UNKNOWN || key < 0 || key >= OK_KEY_COUNT) {
    return false;
  }
  return _physKeys[key];
}

/**
 * @brief Did this combo just happen on the physical keyboard?
 */
bool OkInput::isComboJustPressed(int mods, OkKey key) const {
  return comboJustPressed(_physKeys, _prevPhysKeys, mods, key);
}

/**
 * @brief Hide a key from the game until it is physically released.
 */
void OkInput::consumeKeyUntilReleased(OkKey key) {
  if (key != OK_KEY_UNKNOWN && key >= 0 && key < OK_KEY_COUNT) {
    _consumed[key] = true;
  }
}

void OkInput::process() {
  if (!_window)
    return;

  // Store previous key states
  _prevKeys  = _currentKeys;
  _prevState = _currentState;

  // Before anything is polled: does the person get their keyboard back?
  //
  // Two ways, and both are read straight from GLFW rather than through
  // the gate, because the gate is exactly what they are lifting. A timed
  // block ends on its own, and the release combo ends any block at all --
  // an agent that blocks input and then dies must not leave a window
  // that ignores its owner.
  // The DEVICE first, whether or not the game may see it.
  //
  // Reading it always is what lets the engine recognise a combo while
  // input is blocked -- and the gesture that gives the keyboard back is
  // needed exactly then. The gate below decides who sees what; it is no
  // longer the thing that decides what is read.
  double now    = glfwGetTime();
  _prevPhysKeys = _physKeys;
  for (int i = 0; i < OK_KEY_COUNT; i++) {
    int glfwKey  = OkKeys::okKeyToGLFW(static_cast<OkKey>(i));
    _physKeys[i] = glfwKey != GLFW_KEY_UNKNOWN &&
                   glfwGetKey(_window, glfwKey) == GLFW_PRESS;
  }

  // The press history, from the device and nothing else: what went
  // down this frame, with the modifiers that were already held.
  for (int i = 0; i < OK_KEY_COUNT; i++) {
    if (_physKeys[i] && !_prevPhysKeys[i]) {
      OkKeyPress press;
      press.key  = static_cast<OkKey>(i);
      press.mods = modifiersOf(_physKeys);
      press.when = now;
      _presses.push_back(press);
      if (static_cast<int>(_presses.size()) > PRESS_HISTORY) {
        _presses.erase(_presses.begin());
      }
    }
  }

  if (!_physicalEnabled) {
    if (_blockUntil > 0.0 && now >= _blockUntil) {
      setPhysicalInputEnabled(true);
    } else if (isComboJustPressed(RELEASE_MODS, RELEASE_KEY)) {
      setPhysicalInputEnabled(true);
      // ...and the key of the combo belongs to the combo. Left in the
      // frame, the application reads it a moment later and acts on it:
      // with escape, which this combo used first, that meant handing
      // the keyboard back by quitting the application -- and with a
      // letter it would mean the avatar wandering off on its own.
      consumeKeyUntilReleased(RELEASE_KEY);
    }
  }

  // A consumed key is let go of only when the DEVICE lets go of it.
  for (int i = 0; i < OK_KEY_COUNT; i++) {
    if (_consumed[i] && !_physKeys[i]) {
      _consumed[i] = false;
    }
  }

  // What the game sees: the device when it is allowed to, plus any
  // synthetic (injected) key still within its hold window, minus
  // whatever a combo has taken for itself.
  for (int i = 0; i < OK_KEY_COUNT; i++) {
    bool physical   = _physicalEnabled && _physKeys[i];
    bool injected   = now < _injectedUntil[i];
    _currentKeys[i] = (physical || injected) && !_consumed[i];
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
  for (int i = 0; i < CAMERA_KEYS; i++) {
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
  if (_textCapture) {
    return false;  // the console owns the keyboard
  }
  return isKeyJustPressedRaw(key);
}

/**
 * @brief Raw variant ignoring the text-capture flag (console internals).
 */
bool OkInput::isKeyJustPressedRaw(OkKey key) const {
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
  if (_textCapture) {
    return false;  // the console owns the keyboard
  }
  return isKeyHeldRaw(key);
}

/**
 * @brief Raw variant ignoring the text-capture flag (console internals).
 */
bool OkInput::isKeyHeldRaw(OkKey key) const {
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
  if (_textCapture) {
    return OkInputState();  // neutral state: the console owns the keyboard
  }
  return _currentState;
}

/**
 * @brief Queue a typed character from the GLFW char callback (ASCII only).
 */
void OkInput::onChar(unsigned int codepoint) {
  if (codepoint >= ASCII_FIRST_PRINTABLE && codepoint <= ASCII_LAST_PRINTABLE) {
    _pendingChars.push_back(static_cast<char>(codepoint));
  }
}

/**
 * @brief Return and clear the characters typed since the last call.
 */
std::string OkInput::drainChars() {
  std::string out = _pendingChars;
  _pendingChars.clear();
  return out;
}

/**
 * @brief Mark a key as synthetically held for the next durationSeconds.
 * @param key             The key to inject.
 * @param durationSeconds How long the key should read as pressed.
 */
// How long the injected pointer goes on speaking after the last call
// that touched it. Long enough that a gesture arriving as several MCP
// calls is one drive, short enough that a person reaching for their own
// mouse does not have to wait for it.
static const double POINTER_DRIVE_SECONDS = 2.0;

bool OkInput::injectedPointerUsed() const {
  return _pointerUntil > 0.0 && glfwGetTime() < _pointerUntil;
}

void OkInput::injectPointerTo(double x, double y) {
  _pointerX     = x;
  _pointerY     = y;
  _pointerUntil = glfwGetTime() + POINTER_DRIVE_SECONDS;
}

void OkInput::injectPointerBy(double dx, double dy) {
  _pointerX += dx;
  _pointerY += dy;
  _pointerUntil = glfwGetTime() + POINTER_DRIVE_SECONDS;
}

void OkInput::injectPointerButton(int button, bool down) {
  if (button < 0 || button >= POINTER_BUTTONS) {
    return;
  }
  _pointerDown[button] = down;
  _pointerUntil        = glfwGetTime() + POINTER_DRIVE_SECONDS;
}

void OkInput::injectPointerWheel(double notches) {
  _pointerWheel += notches;
  _pointerUntil = glfwGetTime() + POINTER_DRIVE_SECONDS;
}

void OkInput::injectKey(OkKey key, double durationSeconds) {
  if (key <= OK_KEY_UNKNOWN || key >= OK_KEY_COUNT) {
    return;
  }
  double until = glfwGetTime() + durationSeconds;
  // Extend, never shorten, an existing injection window for this key.
  _injectedUntil[key] = std::max(until, _injectedUntil[key]);
  // While the console owns the keyboard, printable injected keys also feed
  // the typed-character buffer (the GLFW char callback only fires for the
  // physical keyboard, so MCP-injected typing would otherwise be silent).
  if (_textCapture) {
    if (key >= OK_KEY_A && key <= OK_KEY_Z) {
      onChar(static_cast<unsigned int>('a' + (key - OK_KEY_A)));
    } else if (key >= OK_KEY_0 && key <= OK_KEY_9) {
      onChar(static_cast<unsigned int>('0' + (key - OK_KEY_0)));
    } else if (key == OK_KEY_SPACE) {
      onChar(static_cast<unsigned int>(' '));
    } else if (key == OK_KEY_PERIOD) {
      onChar(static_cast<unsigned int>('.'));
    } else if (key == OK_KEY_MINUS) {
      onChar(static_cast<unsigned int>('-'));
    }
  }
}

/**
 * @brief Enable or disable physical keyboard/mouse input.
 * @param enabled True to use real input, false for MCP-only control.
 */
void OkInput::setPhysicalInputEnabled(bool enabled) {
  _physicalEnabled = enabled;
  if (enabled) {
    _blockUntil = 0.0;
  }
  // An application turning input off for a moment is NOT somebody being
  // held out of their own window: an editor does it every time the
  // cursor crosses one of its panels, so that a drag inside a window
  // does not also fly the camera. Only a hold asked for from outside
  // is worth a notice on screen, and `blockPhysicalInput` sets that
  // flag back on after calling this.
  _blockIsHold = false;
  // Pointer lock is opt-in via a click; disabling physical input just makes
  // sure the cursor is released. It is never auto-captured here.
  if (!enabled) {
    setCursorCaptured(false);
  }
}

// The combo that gives the keyboard back, and its name for whoever has
// to tell somebody about it -- one place, so the notice on screen and
// the gesture that works cannot drift apart.
//
// Not escape, which is what it used first: macOS does not deliver
// Escape to an application while Control is held. Measured on the
// device, with the modifiers arriving, the Escape never arriving, and
// ctrl+shift+E arriving in the same breath. A letter with no meaning of
// its own is the safe choice, and the notice is what makes it findable.
const int OkInput::RELEASE_MODS  = OkInput::OK_MOD_CTRL | OkInput::OK_MOD_SHIFT;
const OkKey OkInput::RELEASE_KEY = OK_KEY_K;

const char *OkInput::releaseComboName() {
  return "ctrl+shift+k";
}

const double OkInput::BLOCK_MIN_SECONDS     = 1.0;
const double OkInput::BLOCK_MAX_SECONDS     = 3600.0;
const double OkInput::BLOCK_DEFAULT_SECONDS = 300.0;
const double OkInput::BLOCK_FOREVER         = -1.0;

/**
 * @brief How long a block lasts, given what was asked for.
 * @param seconds the request; zero or less means no deadline.
 * @return the clamped duration, or 0.0 for a block that never expires.
 */
double OkInput::clampBlockSeconds(double seconds) {
  if (seconds <= 0.0) {
    return 0.0;
  }
  if (seconds < BLOCK_MIN_SECONDS) {
    return BLOCK_MIN_SECONDS;
  }
  if (seconds > BLOCK_MAX_SECONDS) {
    return BLOCK_MAX_SECONDS;
  }
  return seconds;
}

/**
 * @brief Ignore physical input for a while, then give it back by itself.
 * @param seconds how long, clamped; zero or less blocks with no deadline.
 */
void OkInput::blockPhysicalInput(double seconds) {
  double hold = clampBlockSeconds(seconds);
  setPhysicalInputEnabled(false);
  _blockUntil  = hold > 0.0 ? glfwGetTime() + hold : 0.0;
  _blockIsHold = true;
}

/**
 * @brief Seconds until physical input returns.
 * @return 0 when input is not blocked, BLOCK_FOREVER for a block with no
 *         deadline, otherwise what is left of it.
 */
double OkInput::physicalInputBlockedFor() const {
  if (_physicalEnabled || !_blockIsHold) {
    return 0.0;
  }
  if (_blockUntil <= 0.0) {
    return BLOCK_FOREVER;
  }
  double left = _blockUntil - glfwGetTime();
  return left > 0.0 ? left : 0.0;
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
      _pointerLockOnClick && _physicalEnabled && !_cursorCaptured &&
      _window != nullptr && glfwGetWindowAttrib(_window, GLFW_FOCUSED) != 0) {
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
