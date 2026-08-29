#include "input_notice.hpp"
#include "../core/core.hpp"
#include "../input/input.hpp"
#include "gui.hpp"
#include "gui_layer.hpp"
#include "gui_text.hpp"
#include <cmath>
#include <string>

// Over the application's own interface but under the console, which is
// the one thing that has to stay readable on top of everything.
static const int NOTICE_LAYER_ORDER = 900;
// Smaller than the console's line: the notice is a state, not something
// to read, and at this height the whole of it fits across a narrow
// window instead of running off the right edge.
static const float NOTICE_TEXT_CELLS = 0.7f;
static const float NOTICE_MARGIN     = 0.5f;

// A plain literal, not a std::string: a string built before main can throw
// where nothing can catch it.
static const char *const NOTICE_LAYER = "ok-input-notice";
static const char *const NOTICE_TEXT  = "ok_input_notice_line";

// Warm amber: a state, not an error.
static const float NOTICE_R = 1.0f;
static const float NOTICE_G = 0.78f;
static const float NOTICE_B = 0.35f;

/**
 * @brief Show or hide the notice, and word it for this frame.
 *
 * Built the first time it is needed rather than at start-up: an
 * application that never blocks input never pays for the layer.
 */
void OkInputNotice::update() {
  double left    = OkCore::userInputBlockedFor();
  bool   blocked = left != 0.0;

  OkGuiLayer *layer = OkGui::getLayer(NOTICE_LAYER);
  if (!blocked) {
    if (layer != nullptr) {
      layer->setVisible(false);
    }
    return;
  }
  if (layer == nullptr) {
    layer            = OkGui::addLayer(NOTICE_LAYER, NOTICE_LAYER_ORDER);
    OkGuiText *built = new OkGuiText(NOTICE_TEXT);
    built->setGridAnchor(OK_GUI_ANCHOR_BOTTOM_LEFT);
    built->setGridHeight(NOTICE_TEXT_CELLS);
    built->setTextColor(NOTICE_R, NOTICE_G, NOTICE_B, 1.0f);
    layer->addItem(built);
  }
  layer->setVisible(true);

  OkGuiText *line = static_cast<OkGuiText *>(layer->getItemByName(NOTICE_TEXT));
  if (line == nullptr) {
    return;
  }
  // The seconds are what makes the notice worth reading: it tells the
  // person the keyboard is coming back on its own, and roughly when.
  // While escape is down the notice counts the hold out loud. That is
  // the point of showing it: the person can SEE the engine reading the
  // key, so a release that does not happen is told apart from a key
  // that never arrived.
  std::string say = "input held by an agent";
  if (left > 0.0) {
    say += ", " + std::to_string(static_cast<int>(std::ceil(left))) + "s";
  }
  say += " -- ctrl+shift+esc frees it";
  line->setText(say);
  line->setGridPosition(NOTICE_MARGIN + line->getGridWidth() * 0.5f,
                        NOTICE_MARGIN + NOTICE_TEXT_CELLS * 0.5f);
}

/**
 * @brief Drop the notice's UI, if it was ever built.
 */
void OkInputNotice::shutdown() {
  OkGui::removeLayer(NOTICE_LAYER);
}
