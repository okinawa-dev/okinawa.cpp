#include "keys.hpp"
#include "../core/gl_config.hpp"

// Lookup table for OkKey to GLFW key mapping
static const int okKeyToGLFWTable[OK_KEY_COUNT] = {
    GLFW_KEY_SPACE,          // OK_KEY_SPACE
    GLFW_KEY_APOSTROPHE,     // OK_KEY_APOSTROPHE
    GLFW_KEY_COMMA,          // OK_KEY_COMMA
    GLFW_KEY_MINUS,          // OK_KEY_MINUS
    GLFW_KEY_PERIOD,         // OK_KEY_PERIOD
    GLFW_KEY_SLASH,          // OK_KEY_SLASH
    GLFW_KEY_0,              // OK_KEY_0
    GLFW_KEY_1,              // OK_KEY_1
    GLFW_KEY_2,              // OK_KEY_2
    GLFW_KEY_3,              // OK_KEY_3
    GLFW_KEY_4,              // OK_KEY_4
    GLFW_KEY_5,              // OK_KEY_5
    GLFW_KEY_6,              // OK_KEY_6
    GLFW_KEY_7,              // OK_KEY_7
    GLFW_KEY_8,              // OK_KEY_8
    GLFW_KEY_9,              // OK_KEY_9
    GLFW_KEY_SEMICOLON,      // OK_KEY_SEMICOLON
    GLFW_KEY_EQUAL,          // OK_KEY_EQUAL
    GLFW_KEY_A,              // OK_KEY_A
    GLFW_KEY_B,              // OK_KEY_B
    GLFW_KEY_C,              // OK_KEY_C
    GLFW_KEY_D,              // OK_KEY_D
    GLFW_KEY_E,              // OK_KEY_E
    GLFW_KEY_F,              // OK_KEY_F
    GLFW_KEY_G,              // OK_KEY_G
    GLFW_KEY_H,              // OK_KEY_H
    GLFW_KEY_I,              // OK_KEY_I
    GLFW_KEY_J,              // OK_KEY_J
    GLFW_KEY_K,              // OK_KEY_K
    GLFW_KEY_L,              // OK_KEY_L
    GLFW_KEY_M,              // OK_KEY_M
    GLFW_KEY_N,              // OK_KEY_N
    GLFW_KEY_O,              // OK_KEY_O
    GLFW_KEY_P,              // OK_KEY_P
    GLFW_KEY_Q,              // OK_KEY_Q
    GLFW_KEY_R,              // OK_KEY_R
    GLFW_KEY_S,              // OK_KEY_S
    GLFW_KEY_T,              // OK_KEY_T
    GLFW_KEY_U,              // OK_KEY_U
    GLFW_KEY_V,              // OK_KEY_V
    GLFW_KEY_W,              // OK_KEY_W
    GLFW_KEY_X,              // OK_KEY_X
    GLFW_KEY_Y,              // OK_KEY_Y
    GLFW_KEY_Z,              // OK_KEY_Z
    GLFW_KEY_LEFT_BRACKET,   // OK_KEY_LEFT_BRACKET
    GLFW_KEY_BACKSLASH,      // OK_KEY_BACKSLASH
    GLFW_KEY_RIGHT_BRACKET,  // OK_KEY_RIGHT_BRACKET
    GLFW_KEY_GRAVE_ACCENT,   // OK_KEY_GRAVE_ACCENT
    GLFW_KEY_ESCAPE,         // OK_KEY_ESCAPE
    GLFW_KEY_ENTER,          // OK_KEY_ENTER
    GLFW_KEY_TAB,            // OK_KEY_TAB
    GLFW_KEY_BACKSPACE,      // OK_KEY_BACKSPACE
    GLFW_KEY_INSERT,         // OK_KEY_INSERT
    GLFW_KEY_DELETE,         // OK_KEY_DELETE
    GLFW_KEY_RIGHT,          // OK_KEY_RIGHT
    GLFW_KEY_LEFT,           // OK_KEY_LEFT
    GLFW_KEY_DOWN,           // OK_KEY_DOWN
    GLFW_KEY_UP,             // OK_KEY_UP
    GLFW_KEY_PAGE_UP,        // OK_KEY_PAGE_UP
    GLFW_KEY_PAGE_DOWN,      // OK_KEY_PAGE_DOWN
    GLFW_KEY_HOME,           // OK_KEY_HOME
    GLFW_KEY_END,            // OK_KEY_END
    GLFW_KEY_CAPS_LOCK,      // OK_KEY_CAPS_LOCK
    GLFW_KEY_SCROLL_LOCK,    // OK_KEY_SCROLL_LOCK
    GLFW_KEY_NUM_LOCK,       // OK_KEY_NUM_LOCK
    GLFW_KEY_PRINT_SCREEN,   // OK_KEY_PRINT_SCREEN
    GLFW_KEY_PAUSE,          // OK_KEY_PAUSE
    GLFW_KEY_F1,             // OK_KEY_F1
    GLFW_KEY_F2,             // OK_KEY_F2
    GLFW_KEY_F3,             // OK_KEY_F3
    GLFW_KEY_F4,             // OK_KEY_F4
    GLFW_KEY_F5,             // OK_KEY_F5
    GLFW_KEY_F6,             // OK_KEY_F6
    GLFW_KEY_F7,             // OK_KEY_F7
    GLFW_KEY_F8,             // OK_KEY_F8
    GLFW_KEY_F9,             // OK_KEY_F9
    GLFW_KEY_F10,            // OK_KEY_F10
    GLFW_KEY_F11,            // OK_KEY_F11
    GLFW_KEY_F12,            // OK_KEY_F12
    GLFW_KEY_KP_0,           // OK_KEY_KP_0
    GLFW_KEY_KP_1,           // OK_KEY_KP_1
    GLFW_KEY_KP_2,           // OK_KEY_KP_2
    GLFW_KEY_KP_3,           // OK_KEY_KP_3
    GLFW_KEY_KP_4,           // OK_KEY_KP_4
    GLFW_KEY_KP_5,           // OK_KEY_KP_5
    GLFW_KEY_KP_6,           // OK_KEY_KP_6
    GLFW_KEY_KP_7,           // OK_KEY_KP_7
    GLFW_KEY_KP_8,           // OK_KEY_KP_8
    GLFW_KEY_KP_9,           // OK_KEY_KP_9
    GLFW_KEY_KP_DECIMAL,     // OK_KEY_KP_DECIMAL
    GLFW_KEY_KP_DIVIDE,      // OK_KEY_KP_DIVIDE
    GLFW_KEY_KP_MULTIPLY,    // OK_KEY_KP_MULTIPLY
    GLFW_KEY_KP_SUBTRACT,    // OK_KEY_KP_SUBTRACT
    GLFW_KEY_KP_ADD,         // OK_KEY_KP_ADD
    GLFW_KEY_KP_ENTER,       // OK_KEY_KP_ENTER
    GLFW_KEY_KP_EQUAL,       // OK_KEY_KP_EQUAL
    GLFW_KEY_LEFT_SHIFT,     // OK_KEY_LEFT_SHIFT
    GLFW_KEY_LEFT_CONTROL,   // OK_KEY_LEFT_CONTROL
    GLFW_KEY_LEFT_ALT,       // OK_KEY_LEFT_ALT
    GLFW_KEY_LEFT_SUPER,     // OK_KEY_LEFT_SUPER
    GLFW_KEY_RIGHT_SHIFT,    // OK_KEY_RIGHT_SHIFT
    GLFW_KEY_RIGHT_CONTROL,  // OK_KEY_RIGHT_CONTROL
    GLFW_KEY_RIGHT_ALT,      // OK_KEY_RIGHT_ALT
    GLFW_KEY_RIGHT_SUPER,    // OK_KEY_RIGHT_SUPER
    GLFW_KEY_MENU            // OK_KEY_MENU
};

// Lookup table for key names
static const char *keyNameTable[OK_KEY_COUNT] = {
    "Space",            // OK_KEY_SPACE
    "Apostrophe",       // OK_KEY_APOSTROPHE
    "Comma",            // OK_KEY_COMMA
    "Minus",            // OK_KEY_MINUS
    "Period",           // OK_KEY_PERIOD
    "Slash",            // OK_KEY_SLASH
    "0",                // OK_KEY_0
    "1",                // OK_KEY_1
    "2",                // OK_KEY_2
    "3",                // OK_KEY_3
    "4",                // OK_KEY_4
    "5",                // OK_KEY_5
    "6",                // OK_KEY_6
    "7",                // OK_KEY_7
    "8",                // OK_KEY_8
    "9",                // OK_KEY_9
    "Semicolon",        // OK_KEY_SEMICOLON
    "Equal",            // OK_KEY_EQUAL
    "A",                // OK_KEY_A
    "B",                // OK_KEY_B
    "C",                // OK_KEY_C
    "D",                // OK_KEY_D
    "E",                // OK_KEY_E
    "F",                // OK_KEY_F
    "G",                // OK_KEY_G
    "H",                // OK_KEY_H
    "I",                // OK_KEY_I
    "J",                // OK_KEY_J
    "K",                // OK_KEY_K
    "L",                // OK_KEY_L
    "M",                // OK_KEY_M
    "N",                // OK_KEY_N
    "O",                // OK_KEY_O
    "P",                // OK_KEY_P
    "Q",                // OK_KEY_Q
    "R",                // OK_KEY_R
    "S",                // OK_KEY_S
    "T",                // OK_KEY_T
    "U",                // OK_KEY_U
    "V",                // OK_KEY_V
    "W",                // OK_KEY_W
    "X",                // OK_KEY_X
    "Y",                // OK_KEY_Y
    "Z",                // OK_KEY_Z
    "Left Bracket",     // OK_KEY_LEFT_BRACKET
    "Backslash",        // OK_KEY_BACKSLASH
    "Right Bracket",    // OK_KEY_RIGHT_BRACKET
    "Grave Accent",     // OK_KEY_GRAVE_ACCENT
    "Escape",           // OK_KEY_ESCAPE
    "Enter",            // OK_KEY_ENTER
    "Tab",              // OK_KEY_TAB
    "Backspace",        // OK_KEY_BACKSPACE
    "Insert",           // OK_KEY_INSERT
    "Delete",           // OK_KEY_DELETE
    "Right Arrow",      // OK_KEY_RIGHT
    "Left Arrow",       // OK_KEY_LEFT
    "Down Arrow",       // OK_KEY_DOWN
    "Up Arrow",         // OK_KEY_UP
    "Page Up",          // OK_KEY_PAGE_UP
    "Page Down",        // OK_KEY_PAGE_DOWN
    "Home",             // OK_KEY_HOME
    "End",              // OK_KEY_END
    "Caps Lock",        // OK_KEY_CAPS_LOCK
    "Scroll Lock",      // OK_KEY_SCROLL_LOCK
    "Num Lock",         // OK_KEY_NUM_LOCK
    "Print Screen",     // OK_KEY_PRINT_SCREEN
    "Pause",            // OK_KEY_PAUSE
    "F1",               // OK_KEY_F1
    "F2",               // OK_KEY_F2
    "F3",               // OK_KEY_F3
    "F4",               // OK_KEY_F4
    "F5",               // OK_KEY_F5
    "F6",               // OK_KEY_F6
    "F7",               // OK_KEY_F7
    "F8",               // OK_KEY_F8
    "F9",               // OK_KEY_F9
    "F10",              // OK_KEY_F10
    "F11",              // OK_KEY_F11
    "F12",              // OK_KEY_F12
    "Keypad 0",         // OK_KEY_KP_0
    "Keypad 1",         // OK_KEY_KP_1
    "Keypad 2",         // OK_KEY_KP_2
    "Keypad 3",         // OK_KEY_KP_3
    "Keypad 4",         // OK_KEY_KP_4
    "Keypad 5",         // OK_KEY_KP_5
    "Keypad 6",         // OK_KEY_KP_6
    "Keypad 7",         // OK_KEY_KP_7
    "Keypad 8",         // OK_KEY_KP_8
    "Keypad 9",         // OK_KEY_KP_9
    "Keypad Decimal",   // OK_KEY_KP_DECIMAL
    "Keypad Divide",    // OK_KEY_KP_DIVIDE
    "Keypad Multiply",  // OK_KEY_KP_MULTIPLY
    "Keypad Subtract",  // OK_KEY_KP_SUBTRACT
    "Keypad Add",       // OK_KEY_KP_ADD
    "Keypad Enter",     // OK_KEY_KP_ENTER
    "Keypad Equal",     // OK_KEY_KP_EQUAL
    "Left Shift",       // OK_KEY_LEFT_SHIFT
    "Left Control",     // OK_KEY_LEFT_CONTROL
    "Left Alt",         // OK_KEY_LEFT_ALT
    "Left Super",       // OK_KEY_LEFT_SUPER
    "Right Shift",      // OK_KEY_RIGHT_SHIFT
    "Right Control",    // OK_KEY_RIGHT_CONTROL
    "Right Alt",        // OK_KEY_RIGHT_ALT
    "Right Super",      // OK_KEY_RIGHT_SUPER
    "Menu"              // OK_KEY_MENU
};

/**
 * @brief Convert an OkKey to the corresponding OpenGL/GLFW key code.
 * @param key The OkKey to convert.
 * @return The corresponding GLFW key code, or GLFW_KEY_UNKNOWN if not found.
 */
int OkKeys::okKeyToGLFW(OkKey key) {
  if (key == OK_KEY_UNKNOWN || key < 0 || key >= OK_KEY_COUNT) {
    return GLFW_KEY_UNKNOWN;
  }
  return okKeyToGLFWTable[key];
}

/**
 * @brief Convert a GLFW key code to the corresponding OkKey.
 * @param glfwKey The GLFW key code to convert.
 * @return The corresponding OkKey, or OK_KEY_UNKNOWN if not found.
 */
OkKey OkKeys::glfwToOkKey(int glfwKey) {
  if (glfwKey == GLFW_KEY_UNKNOWN) {
    return OK_KEY_UNKNOWN;
  }

  // Linear search through the lookup table
  for (int i = 0; i < OK_KEY_COUNT; i++) {
    if (okKeyToGLFWTable[i] == glfwKey) {
      return static_cast<OkKey>(i);
    }
  }

  return OK_KEY_UNKNOWN;
}

/**
 * @brief Get the name of a key as a string.
 * @param key The OkKey to get the name for.
 * @return The name of the key as a string.
 */
const char *OkKeys::getKeyName(OkKey key) {
  if (key == OK_KEY_UNKNOWN || key < 0 || key >= OK_KEY_COUNT) {
    return "Unknown";
  }
  return keyNameTable[key];
}

/**
 * @brief Check if a key is a modifier key (Shift, Ctrl, Alt, etc.).
 * @param key The OkKey to check.
 * @return True if the key is a modifier key, false otherwise.
 */
bool OkKeys::isModifierKey(OkKey key) {
  switch (key) {
    case OK_KEY_LEFT_SHIFT:
    case OK_KEY_LEFT_CONTROL:
    case OK_KEY_LEFT_ALT:
    case OK_KEY_LEFT_SUPER:
    case OK_KEY_RIGHT_SHIFT:
    case OK_KEY_RIGHT_CONTROL:
    case OK_KEY_RIGHT_ALT:
    case OK_KEY_RIGHT_SUPER:
      return true;
    default:
      return false;
  }
}

/**
 * @brief Check if a key is a function key (F1-F12).
 * @param key The OkKey to check.
 * @return True if the key is a function key, false otherwise.
 */
bool OkKeys::isFunctionKey(OkKey key) {
  return (key >= OK_KEY_F1 && key <= OK_KEY_F12);
}

/**
 * @brief Check if a key is a letter key (A-Z).
 * @param key The OkKey to check.
 * @return True if the key is a letter key, false otherwise.
 */
bool OkKeys::isLetterKey(OkKey key) {
  return (key >= OK_KEY_A && key <= OK_KEY_Z);
}

/**
 * @brief Check if a key is a number key (0-9).
 * @param key The OkKey to check.
 * @return True if the key is a number key, false otherwise.
 */
bool OkKeys::isNumberKey(OkKey key) {
  return (key >= OK_KEY_0 && key <= OK_KEY_9);
}
