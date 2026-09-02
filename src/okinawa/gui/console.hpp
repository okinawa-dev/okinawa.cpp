#ifndef OK_CONSOLE_HPP
#define OK_CONSOLE_HPP

#include <functional>
#include <string>
#include <vector>

/**
 * @brief Quake-style drop-down command console. Toggled with the grave key
 *        (`), it covers the top half of the screen (a GUI layer of its own)
 *        and captures ALL keyboard input while open: the game sees nothing,
 *        so typing commands cannot trigger gameplay keys.
 *
 *        The command set is EXTENSIBLE: the engine registers a few
 *        built-ins (help, clear, quit, set/get over OkConfig) and every
 *        game adds its own with registerCommand — a name, a help line and
 *        a callback receiving the parsed arguments. Commands answer back
 *        through print().
 */
class OkConsole {
public:
  // Command callback: receives the arguments (argv[0] is the first
  // argument, NOT the command name).
  using OkConsoleCommand =
      std::function<void(const std::vector<std::string> &args)>;

  OkConsole() = delete;

  // Register built-in commands. Called by OkCore::initialize.
  static void initialize();

  // Destroy UI resources. Called by OkCore::exit.
  static void shutdown();

  // Register / list commands. Registering an existing name replaces it.
  static void registerCommand(const std::string &name, const std::string &help,
                              const OkConsoleCommand &callback);
  static std::vector<std::string> getCommandNames();

  // Parse and run a command line ("name arg1 arg2 ..."); echoes the line
  // and any errors to the output buffer.
  static void execute(const std::string &line);

  // Append a line to the console output buffer.
  static void print(const std::string &line);

  // Read back what commands wrote. The scrollback is trimmed, so the line
  // count is not the number of lines still held: getPrintedCount returns
  // how many lines have been printed since the console was created, and
  // getOutputTail returns the newest lines still in the buffer, oldest
  // first. Take the count before and after an execute() and the difference
  // is that command's answer.
  static unsigned long getPrintedCount() {
    return _printed;
  }
  static std::vector<std::string> getOutputTail(int maxLines);

  // The lines submitted so far, oldest first. An application drawing a
  // console of its own needs them: walking back through what you typed
  // is half of what a console IS, and a second history kept beside this
  // one would forget everything typed into the other.
  static const std::vector<std::string> &getHistory() {
    return _history;
  }

  // Whether the console answers its key at all.
  //
  // An application with its own interface does not want a second one
  // opening over it: the editor draws its panels with Dear ImGui and a
  // grave typed into a text field there must stay a grave. Off, the
  // console is still there to be driven -- `execute` and the output
  // buffer are what an interface of one's own is built on -- it simply
  // has no key of its own and never draws itself.
  static void setKeyEnabled(bool enabled);
  static bool isKeyEnabled() {
    return _keyEnabled;
  }

  // Open state. While open the console owns the keyboard (OkInput is
  // captured) and the game receives no keys.
  static void toggle();
  static bool isOpen() {
    return _open;
  }

  // Per-frame update: handles the toggle key, line editing and history
  // while open, and refreshes the UI. Called by OkCore::loop right after
  // input processing.
  static void update(float dt);

private:
  struct Command {
    std::string      name;
    std::string      help;
    OkConsoleCommand callback;
  };

  // Split a command line into whitespace-separated tokens.
  static std::vector<std::string> tokenize(const std::string &line);

  // Lazy UI construction and per-frame refresh of the visible lines.
  static void ensureUi();
  static void refreshUi();

  static bool                     _open;
  static bool                     _keyEnabled;
  static std::vector<Command>     _commands;
  static std::vector<std::string> _output;   // scrollback, newest last
  static std::vector<std::string> _history;  // submitted lines
  static int                      _historyPos;
  static unsigned long            _printed;  // lines printed, ever
  static std::string              _input;    // line being typed
  static float                    _blinkT;   // cursor blink accumulator
  static bool                     _uiBuilt;
};

#endif
