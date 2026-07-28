#include "console.hpp"
#include "../config/config.hpp"
#include "../core/core.hpp"
#include "../input/input.hpp"
#include "../utils/logger.hpp"
#include "gui.hpp"
#include "gui_image.hpp"
#include "gui_layer.hpp"
#include "gui_text.hpp"
#include <cctype>
#include <cstdlib>
#include <sstream>

bool                              OkConsole::_open = false;
std::vector<OkConsole::Command>   OkConsole::_commands;
std::vector<std::string>          OkConsole::_output;
std::vector<std::string>          OkConsole::_history;
int                               OkConsole::_historyPos = -1;
std::string                       OkConsole::_input;
float                             OkConsole::_blinkT  = 0.0f;
bool                              OkConsole::_uiBuilt = false;

// UI constants: layer order (over everything), visible output lines, text
// height and margins in grid cells, scrollback cap and cursor blink rate.
// NOLINTBEGIN(readability-magic-numbers)
static const int   CONSOLE_LAYER_ORDER = 1000;
static const int   CONSOLE_LINES       = 12;
static const float CONSOLE_TEXT_CELLS  = 0.8f;
static const float CONSOLE_MARGIN      = 0.5f;
static const int   CONSOLE_SCROLLBACK  = 200;
static const float CONSOLE_BLINK_S     = 0.5f;
// NOLINTEND(readability-magic-numbers)

static const std::string CONSOLE_LAYER = "ok-console";

/**
 * @brief Register the engine built-in commands.
 */
void OkConsole::initialize() {
  registerCommand("help", "list the available commands",
                  [](const std::vector<std::string> &args) {
                    (void)args;
                    std::vector<std::string> names =
                        OkConsole::getCommandNames();
                    for (std::size_t i = 0; i < names.size(); i++) {
                      OkConsole::print("  " + names[i]);
                    }
                  });
  registerCommand("clear", "clear the console output",
                  [](const std::vector<std::string> &args) {
                    (void)args;
                    OkConsole::_output.clear();
                  });
  registerCommand("quit", "exit the application",
                  [](const std::vector<std::string> &args) {
                    (void)args;
                    OkCore::askForExit();
                  });
  registerCommand(
      "set", "set <config-key> <value>: write an engine config value",
      [](const std::vector<std::string> &args) {
        if (args.size() != 2) {
          OkConsole::print("usage: set <config-key> <value>");
          return;
        }
        const std::string &key = args[0];
        const std::string &val = args[1];
        if (val == "true" || val == "false") {
          OkConfig::setBool(key, val == "true");
        } else if (val.find('.') != std::string::npos) {
          OkConfig::setFloat(key, (float)atof(val.c_str()));
        } else if (!val.empty() &&
                   (isdigit((unsigned char)val[0]) || val[0] == '-')) {
          OkConfig::setInt(key, atoi(val.c_str()));
        } else {
          OkConfig::setString(key, val);
        }
        OkConsole::print(key + " = " + val);
      });
  registerCommand(
      "get", "get <config-key>: read an engine config value",
      [](const std::vector<std::string> &args) {
        if (args.size() != 1) {
          OkConsole::print("usage: get <config-key>");
          return;
        }
        const std::string &key = args[0];
        // The config stores each type in its own map; probe them all.
        std::ostringstream out;
        out << args[0] << ": int=" << OkConfig::getInt(key)
            << " float=" << OkConfig::getFloat(key)
            << " bool=" << (OkConfig::getBool(key) ? "true" : "false");
        std::string str = OkConfig::getString(key);
        if (!str.empty()) {
          out << " string=" << str;
        }
        OkConsole::print(out.str());
      });

  print("okinawa console. type 'help' for commands.");
}

void OkConsole::shutdown() {
  _commands.clear();
  _output.clear();
  _history.clear();
  _uiBuilt = false;
  _open    = false;
}

/**
 * @brief Register (or replace) a command.
 */
void OkConsole::registerCommand(const std::string      &name,
                                const std::string      &help,
                                const OkConsoleCommand &callback) {
  for (std::size_t i = 0; i < _commands.size(); i++) {
    if (_commands[i].name == name) {
      _commands[i].help     = help;
      _commands[i].callback = callback;
      return;
    }
  }
  Command cmd;
  cmd.name     = name;
  cmd.help     = help;
  cmd.callback = callback;
  _commands.push_back(cmd);
}

std::vector<std::string> OkConsole::getCommandNames() {
  std::vector<std::string> names;
  for (std::size_t i = 0; i < _commands.size(); i++) {
    names.push_back(_commands[i].name + " - " + _commands[i].help);
  }
  return names;
}

/**
 * @brief Whitespace tokenizer for command lines.
 */
std::vector<std::string> OkConsole::tokenize(const std::string &line) {
  std::vector<std::string> tokens;
  std::istringstream       in(line);
  std::string              tok;
  while (in >> tok) {
    tokens.push_back(tok);
  }
  return tokens;
}

/**
 * @brief Parse and run a command line; echoes the line and reports unknown
 *        commands. The submitted line joins the history.
 */
void OkConsole::execute(const std::string &line) {
  std::vector<std::string> tokens = tokenize(line);
  if (tokens.empty()) {
    return;
  }

  print("> " + line);
  _history.push_back(line);
  _historyPos = -1;

  std::string name = tokens[0];
  for (std::size_t i = 0; i < _commands.size(); i++) {
    if (_commands[i].name == name) {
      std::vector<std::string> args(tokens.begin() + 1, tokens.end());
      _commands[i].callback(args);
      return;
    }
  }
  print("unknown command: " + name + " (try 'help')");
}

void OkConsole::print(const std::string &line) {
  _output.push_back(line);
  if ((int)_output.size() > CONSOLE_SCROLLBACK) {
    _output.erase(_output.begin(),
                  _output.begin() +
                      ((long)_output.size() - CONSOLE_SCROLLBACK));
  }
}

void OkConsole::toggle() {
  _open = !_open;
  OkInput *input = OkCore::getInput();
  if (input != nullptr) {
    input->setTextCapture(_open);
    if (_open) {
      input->drainChars();  // discard anything typed before opening
    }
  }
  OkGuiLayer *layer = OkGui::getLayer(CONSOLE_LAYER);
  if (layer != nullptr) {
    layer->setVisible(_open);
  }
}

/**
 * @brief Build the console UI once: a dark plate over the top half of the
 *        screen plus the output lines and the input line, all TOP-anchored
 *        so they stay put across window sizes.
 */
void OkConsole::ensureUi() {
  if (_uiBuilt) {
    return;
  }
  _uiBuilt = true;

  OkGuiLayer *layer = OkGui::addLayer(CONSOLE_LAYER, CONSOLE_LAYER_ORDER);
  layer->setVisible(_open);

  OkGuiImage *plate = new OkGuiImage("ok_console_plate");
  plate->setGridAnchor(OK_GUI_ANCHOR_TOP);
  plate->setFillColor(0.05f, 0.07f, 0.10f, 0.85f);
  layer->addItem(plate);

  for (int i = 0; i < CONSOLE_LINES; i++) {
    OkGuiText *line = new OkGuiText("ok_console_line" + std::to_string(i));
    line->setGridAnchor(OK_GUI_ANCHOR_TOP_LEFT);
    line->setGridHeight(CONSOLE_TEXT_CELLS);
    line->setTextColor(0.85f, 0.9f, 0.95f, 1.0f);
    layer->addItem(line);
  }

  OkGuiText *prompt = new OkGuiText("ok_console_input");
  prompt->setGridAnchor(OK_GUI_ANCHOR_TOP_LEFT);
  prompt->setGridHeight(CONSOLE_TEXT_CELLS);
  prompt->setTextColor(1.0f, 0.9f, 0.4f, 1.0f);
  layer->addItem(prompt);
}

/**
 * @brief Refresh the UI: size the plate to the window, lay the last output
 *        lines top-down and the input line (with blinking cursor) under
 *        them. Grid units everywhere, so gui.scale keeps working.
 */
void OkConsole::refreshUi() {
  OkGuiLayer *layer = OkGui::getLayer(CONSOLE_LAYER);
  if (layer == nullptr) {
    return;
  }

  // Window size in grid cells (anchor origins are half the window).
  float logicalW =
      OkGui::screenToGridX(OkGui::anchorOriginX(OK_GUI_ANCHOR_RIGHT) * 2.0f);
  float logicalH =
      OkGui::screenToGridY(OkGui::anchorOriginY(OK_GUI_ANCHOR_TOP) * 2.0f);

  float lineStep = CONSOLE_TEXT_CELLS + 0.2f;
  float plateH   = (float)(CONSOLE_LINES + 1) * lineStep + CONSOLE_MARGIN * 3.0f;

  // The half-screen classic: never taller than half the window.
  if (plateH > logicalH * 0.5f) {
    plateH = logicalH * 0.5f;
  }

  OkGuiImage *plate =
      (OkGuiImage *)layer->getItemByName("ok_console_plate");
  if (plate != nullptr) {
    plate->setGridSize(logicalW, plateH);
    plate->setGridPosition(0.0f, -plateH * 0.5f);
  }

  // Output lines, oldest at the top, newest just above the input line.
  int total = (int)_output.size();
  for (int i = 0; i < CONSOLE_LINES; i++) {
    OkGuiText *line = (OkGuiText *)layer->getItemByName(
        "ok_console_line" + std::to_string(i));
    if (line == nullptr) {
      continue;
    }
    int src = total - CONSOLE_LINES + i;
    std::string text = (src >= 0) ? _output[(std::size_t)src] : "";
    line->setText(text);
    float y = -CONSOLE_MARGIN - (float)i * lineStep -
              CONSOLE_TEXT_CELLS * 0.5f;
    line->setGridPosition(CONSOLE_MARGIN + line->getGridWidth() * 0.5f, y);
  }

  OkGuiText *prompt =
      (OkGuiText *)layer->getItemByName("ok_console_input");
  if (prompt != nullptr) {
    bool        cursorOn = ((int)(_blinkT / CONSOLE_BLINK_S) % 2) == 0;
    std::string text     = "> " + _input + (cursorOn ? "_" : " ");
    prompt->setText(text);
    float y = -CONSOLE_MARGIN - (float)CONSOLE_LINES * lineStep -
              CONSOLE_TEXT_CELLS * 0.5f;
    prompt->setGridPosition(CONSOLE_MARGIN + prompt->getGridWidth() * 0.5f,
                            y);
  }
}

/**
 * @brief Per-frame console driver: toggle key, then (while open) typed
 *        characters, line editing, history and UI refresh.
 */
void OkConsole::update(float dt) {
  OkInput *input = OkCore::getInput();
  if (input == nullptr) {
    return;
  }

  if (input->isKeyJustPressedRaw(OK_KEY_GRAVE_ACCENT)) {
    ensureUi();
    toggle();
  }
  if (!_open) {
    return;
  }

  _blinkT += dt / 1000.0f;

  // Typed characters (the grave that toggles the console is filtered out).
  std::string typed = input->drainChars();
  for (std::size_t i = 0; i < typed.size(); i++) {
    if (typed[i] != '`') {
      _input.push_back(typed[i]);
    }
  }

  if (input->isKeyJustPressedRaw(OK_KEY_BACKSPACE) && !_input.empty()) {
    _input.erase(_input.size() - 1);
  }
  if (input->isKeyJustPressedRaw(OK_KEY_ESCAPE)) {
    toggle();
    return;
  }
  if (input->isKeyJustPressedRaw(OK_KEY_ENTER) ||
      input->isKeyJustPressedRaw(OK_KEY_KP_ENTER)) {
    std::string line = _input;
    _input.clear();
    execute(line);
  }

  // History: up/down walk the submitted lines (newest first).
  if (input->isKeyJustPressedRaw(OK_KEY_UP) && !_history.empty()) {
    if (_historyPos < (int)_history.size() - 1) {
      _historyPos++;
    }
    _input = _history[_history.size() - 1 - (std::size_t)_historyPos];
  }
  if (input->isKeyJustPressedRaw(OK_KEY_DOWN)) {
    if (_historyPos > 0) {
      _historyPos--;
      _input = _history[_history.size() - 1 - (std::size_t)_historyPos];
    } else if (_historyPos == 0) {
      _historyPos = -1;
      _input.clear();
    }
  }

  refreshUi();
}
