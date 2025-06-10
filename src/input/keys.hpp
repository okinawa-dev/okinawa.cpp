#ifndef OK_KEYS_HPP
#define OK_KEYS_HPP

#include <cstdint>

/**
 * @brief Enumeration of all possible keyboard keys for the Okinawa engine.
 *        This provides a platform-independent way to reference keyboard keys.
 */
enum OkKey : std::int8_t {
  // Special keys
  OK_KEY_UNKNOWN = -1,
  OK_KEY_SPACE,
  OK_KEY_APOSTROPHE, /* ' */
  OK_KEY_COMMA,      /* , */
  OK_KEY_MINUS,      /* - */
  OK_KEY_PERIOD,     /* . */
  OK_KEY_SLASH,      /* / */

  // Numbers
  OK_KEY_0,
  OK_KEY_1,
  OK_KEY_2,
  OK_KEY_3,
  OK_KEY_4,
  OK_KEY_5,
  OK_KEY_6,
  OK_KEY_7,
  OK_KEY_8,
  OK_KEY_9,

  // Special characters
  OK_KEY_SEMICOLON, /* ; */
  OK_KEY_EQUAL,     /* = */

  // Letters A-Z
  OK_KEY_A,
  OK_KEY_B,
  OK_KEY_C,
  OK_KEY_D,
  OK_KEY_E,
  OK_KEY_F,
  OK_KEY_G,
  OK_KEY_H,
  OK_KEY_I,
  OK_KEY_J,
  OK_KEY_K,
  OK_KEY_L,
  OK_KEY_M,
  OK_KEY_N,
  OK_KEY_O,
  OK_KEY_P,
  OK_KEY_Q,
  OK_KEY_R,
  OK_KEY_S,
  OK_KEY_T,
  OK_KEY_U,
  OK_KEY_V,
  OK_KEY_W,
  OK_KEY_X,
  OK_KEY_Y,
  OK_KEY_Z,

  // Brackets and backslash
  OK_KEY_LEFT_BRACKET,  /* [ */
  OK_KEY_BACKSLASH,     /* \ */
  OK_KEY_RIGHT_BRACKET, /* ] */
  OK_KEY_GRAVE_ACCENT,  /* ` */

  // Function keys
  OK_KEY_ESCAPE,
  OK_KEY_ENTER,
  OK_KEY_TAB,
  OK_KEY_BACKSPACE,
  OK_KEY_INSERT,
  OK_KEY_DELETE,
  OK_KEY_RIGHT,
  OK_KEY_LEFT,
  OK_KEY_DOWN,
  OK_KEY_UP,
  OK_KEY_PAGE_UP,
  OK_KEY_PAGE_DOWN,
  OK_KEY_HOME,
  OK_KEY_END,
  OK_KEY_CAPS_LOCK,
  OK_KEY_SCROLL_LOCK,
  OK_KEY_NUM_LOCK,
  OK_KEY_PRINT_SCREEN,
  OK_KEY_PAUSE,

  // Function keys F1-F12
  OK_KEY_F1,
  OK_KEY_F2,
  OK_KEY_F3,
  OK_KEY_F4,
  OK_KEY_F5,
  OK_KEY_F6,
  OK_KEY_F7,
  OK_KEY_F8,
  OK_KEY_F9,
  OK_KEY_F10,
  OK_KEY_F11,
  OK_KEY_F12,

  // Keypad
  OK_KEY_KP_0,
  OK_KEY_KP_1,
  OK_KEY_KP_2,
  OK_KEY_KP_3,
  OK_KEY_KP_4,
  OK_KEY_KP_5,
  OK_KEY_KP_6,
  OK_KEY_KP_7,
  OK_KEY_KP_8,
  OK_KEY_KP_9,
  OK_KEY_KP_DECIMAL,
  OK_KEY_KP_DIVIDE,
  OK_KEY_KP_MULTIPLY,
  OK_KEY_KP_SUBTRACT,
  OK_KEY_KP_ADD,
  OK_KEY_KP_ENTER,
  OK_KEY_KP_EQUAL,

  // Modifier keys
  OK_KEY_LEFT_SHIFT,
  OK_KEY_LEFT_CONTROL,
  OK_KEY_LEFT_ALT,
  OK_KEY_LEFT_SUPER,
  OK_KEY_RIGHT_SHIFT,
  OK_KEY_RIGHT_CONTROL,
  OK_KEY_RIGHT_ALT,
  OK_KEY_RIGHT_SUPER,
  OK_KEY_MENU,

  // Total count of keys
  OK_KEY_COUNT
};

/**
 * @brief Class to handle keyboard key mapping and translation.
 *        Provides methods to convert between OkKey enum and OpenGL/GLFW key
 * codes.
 */
class OkKeys {
public:
  /**
   * @brief Convert an OkKey to the corresponding OpenGL/GLFW key code.
   * @param key The OkKey to convert.
   * @return The corresponding GLFW key code, or GLFW_KEY_UNKNOWN if not found.
   */
  static int okKeyToGLFW(OkKey key);

  /**
   * @brief Convert a GLFW key code to the corresponding OkKey.
   * @param glfwKey The GLFW key code to convert.
   * @return The corresponding OkKey, or OK_KEY_UNKNOWN if not found.
   */
  static OkKey glfwToOkKey(int glfwKey);

  /**
   * @brief Get the name of a key as a string.
   * @param key The OkKey to get the name for.
   * @return The name of the key as a string.
   */
  static const char *getKeyName(OkKey key);

  /**
   * @brief Check if a key is a modifier key (Shift, Ctrl, Alt, etc.).
   * @param key The OkKey to check.
   * @return True if the key is a modifier key, false otherwise.
   */
  static bool isModifierKey(OkKey key);

  /**
   * @brief Check if a key is a function key (F1-F12).
   * @param key The OkKey to check.
   * @return True if the key is a function key, false otherwise.
   */
  static bool isFunctionKey(OkKey key);

  /**
   * @brief Check if a key is a letter key (A-Z).
   * @param key The OkKey to check.
   * @return True if the key is a letter key, false otherwise.
   */
  static bool isLetterKey(OkKey key);

  /**
   * @brief Check if a key is a number key (0-9).
   * @param key The OkKey to check.
   * @return True if the key is a number key, false otherwise.
   */
  static bool isNumberKey(OkKey key);
};

#endif
