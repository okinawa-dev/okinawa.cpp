#include "stats.hpp"

#include "../config/config.hpp"
#include "../core/core.hpp"
#include "../handlers/scenes.hpp"
#include "../handlers/textures.hpp"
#include "../item/texture.hpp"
#include "../lighting/lighting.hpp"
#include "../math/frustum.hpp"
#include "../scene/scene.hpp"
#include "../utils/logger.hpp"
#include "console.hpp"
#include "gui.hpp"
#include "gui_image.hpp"
#include "gui_layer.hpp"
#include "gui_text.hpp"
#include <algorithm>
#include <cstdio>

OkGuiLayer *OkGuiStats::_layer    = nullptr;
OkGuiText  *OkGuiStats::_lines[6] = {nullptr, nullptr, nullptr,
                                     nullptr, nullptr, nullptr};
OkGuiImage *OkGuiStats::_graph    = nullptr;
OkTexture  *OkGuiStats::_graphTex = nullptr;

std::vector<float> OkGuiStats::_history;
std::vector<float> OkGuiStats::_drawHistory;
float              OkGuiStats::_accum   = 0.0f;
float              OkGuiStats::_worst   = 0.0f;
bool               OkGuiStats::_visible = false;
// Several seconds even at a high frame rate: long enough to judge a
// change rather than catch a lucky frame.
int                OkGuiStats::_historyMax = 600;

namespace {

// Panel geometry, in grid cells.
const float PANEL_X    = 1.0f;
const float PANEL_Y    = -1.0f;
const float LINE_H     = 0.85f;
const int   GRAPH_W    = 96;    // history samples kept and drawn
const int   GRAPH_H    = 32;    // graph texture height in pixels
// Top of the graph. It adapts to the worst sample in the window, with a
// floor of two 60Hz frames, so the strip stays informative whether the
// project runs at 200 fps or struggles at 20.
const float GRAPH_MS_MIN = 33.4f;
// Readings are refreshed a few times a second: text that changes every
// frame is unreadable, and rebuilding the meshes has its own cost.
const float REFRESH_MS = 180.0f;

}  // namespace

/**
 * @brief Build the panel: a stack of text lines and a frame-time graph
 *        on its own layer, anchored to the top-left corner.
 */
void OkGuiStats::initialize() {
  if (_layer != nullptr) {
    return;
  }
  // A high order keeps the panel above ordinary interface layers, and
  // the console (which sits higher still) above the panel.
  _layer = OkGui::addLayer("ok-stats", 900);
  if (_layer == nullptr) {
    return;
  }

  _graph = new OkGuiImage("ok-stats-graph");
  _graph->setGridAnchor(OK_GUI_ANCHOR_TOP_LEFT);
  _graph->setGridSize((float)GRAPH_W / 12.0f, 1.6f);
  _graph->setGridPosition(PANEL_X + (float)GRAPH_W / 24.0f,
                          PANEL_Y - LINE_H * 6.4f);
  _layer->addItem(_graph);

  for (int i = 0; i < 6; i++) {
    _lines[i] = new OkGuiText("ok-stats-line" + std::to_string(i));
    _lines[i]->setGridAnchor(OK_GUI_ANCHOR_TOP_LEFT);
    _lines[i]->setGridPosition(PANEL_X, PANEL_Y - LINE_H * (float)i);
    _lines[i]->setGridHeight(0.8f);
    _lines[i]->setTextColor(0.86f, 0.92f, 0.86f, 1.0f);
    _lines[i]->setText("");
    _layer->addItem(_lines[i]);
  }
  _layer->setVisible(_visible);

  OkConsole::registerCommand(
      "stats", "stats [on|off]: toggle the runtime statistics panel",
      [](const std::vector<std::string> &args) {
        if (args.empty()) {
          OkGuiStats::setVisible(!OkGuiStats::isVisible());
        } else {
          OkGuiStats::setVisible(args[0] == "on" || args[0] == "1" ||
                                 args[0] == "true");
        }
        OkConsole::print(std::string("stats ") +
                         (OkGuiStats::isVisible() ? "on" : "off"));
      });
  OkLogger::info("GuiStats", "Statistics panel ready (console: stats)");
}

/**
 * @brief Redraw the frame-time strip: one column per sample, its height
 *        proportional to that frame's cost, coloured by how close it
 *        came to missing a refresh.
 */
void OkGuiStats::rebuildGraph() {
  if (_graph == nullptr) {
    return;
  }
  unsigned char rgba[GRAPH_W * GRAPH_H * 4];
  for (int y = 0; y < GRAPH_H; y++) {
    for (int x = 0; x < GRAPH_W; x++) {
      int off       = (y * GRAPH_W + x) * 4;
      rgba[off]     = 12;
      rgba[off + 1] = 16;
      rgba[off + 2] = 18;
      rgba[off + 3] = 170;
    }
  }
  // The graph shows the most recent GRAPH_W samples; the history behind
  // it is longer and is there for callers asking for a series.
  size_t from = _history.size() > (size_t)GRAPH_W
                    ? _history.size() - (size_t)GRAPH_W
                    : 0;
  float top = GRAPH_MS_MIN;
  for (size_t i = from; i < _history.size(); i++) {
    if (_history[i] > top) {
      top = _history[i];
    }
  }
  top *= 1.1f;

  // Reference line at 16.7 ms (60 Hz): the bar to stay under.
  int refY = GRAPH_H - 1 - (int)((16.7f / top) * (GRAPH_H - 1));
  if (refY >= 0 && refY < GRAPH_H) {
    for (int x = 0; x < GRAPH_W; x++) {
      int off       = (refY * GRAPH_W + x) * 4;
      rgba[off]     = 70;
      rgba[off + 1] = 90;
      rgba[off + 2] = 70;
      rgba[off + 3] = 210;
    }
  }

  int n = (int)_history.size();
  for (int i = 0; i < n && i < GRAPH_W; i++) {
    float ms = _history[(size_t)(n - 1 - i)];
    (void)from;
    int   x  = GRAPH_W - 1 - i;
    float t  = ms / top;
    if (t > 1.0f) {
      t = 1.0f;
    }
    int h = (int)(t * (GRAPH_H - 1));
    // Green while comfortably inside a 60Hz budget, amber past it, red
    // once a frame costs more than two refreshes.
    unsigned char r = 90, g = 200, b = 110;
    if (ms > 16.7f) {
      r = 220;
      g = 190;
      b = 90;
    }
    if (ms > 33.4f) {
      r = 225;
      g = 90;
      b = 80;
    }
    for (int y = GRAPH_H - 1; y >= GRAPH_H - 1 - h; y--) {
      int off       = (y * GRAPH_W + x) * 4;
      rgba[off]     = r;
      rgba[off + 1] = g;
      rgba[off + 2] = b;
      rgba[off + 3] = 235;
    }
  }

  if (_graphTex == nullptr) {
    _graphTex = OkTextureHandler::getInstance()->createTextureFromRawData(
        "ok_stats_graph", rgba, GRAPH_W, GRAPH_H, 4);
    if (_graphTex != nullptr) {
      _graphTex->setNearestFiltering();
      _graph->setTexture("ok_stats_graph", _graphTex);
    }
  } else {
    _graphTex->updateRawData(rgba, GRAPH_W, GRAPH_H);
  }
}

void OkGuiStats::update(float dtMs) {
  if (_layer == nullptr) {
    return;
  }
  // The history is kept even while hidden, so opening the panel shows
  // what just happened instead of starting blank.
  _history.push_back(dtMs);
  if ((int)_history.size() > _historyMax) {
    // Drop the oldest in one go rather than one per frame, so the cost
    // does not fall on every single frame.
    _history.erase(_history.begin(),
                   _history.begin() + (_history.size() - _historyMax));
  }
  if (dtMs > _worst) {
    _worst = dtMs;
  }
  if (!_visible) {
    return;
  }

  _accum += dtMs;
  if (_accum < REFRESH_MS) {
    return;
  }
  _accum = 0.0f;

  // Averages over the whole window, not the last frame: a single frame
  // bounces too much to read.
  size_t avgFrom = _history.size() > (size_t)GRAPH_W
                       ? _history.size() - (size_t)GRAPH_W
                       : 0;
  float  sum     = 0.0f;
  for (size_t i = avgFrom; i < _history.size(); i++) {
    sum += _history[i];
  }
  size_t avgN = _history.size() - avgFrom;
  float  avg  = avgN == 0 ? 0.0f : sum / (float)avgN;
  float fps = avg > 0.0001f ? 1000.0f / avg : 0.0f;

  OkSceneHandler *sh    = OkCore::getSceneHandler();
  OkScene        *scene = sh ? sh->getCurrentScene() : nullptr;
  long objects = scene ? (long)scene->getObjectCount() : 0;
  long culled  = OkFrustum::getCulledCount();
  long draws   = OkFrustum::getDrawCalls();
  long tris    = OkFrustum::getTriangles();
  int  texes   = (int)OkTextureHandler::getInstance()->getTextureNames().size();

  char buf[128];
  const char *text[6];
  char        store[6][128];
  // Draw time next to frame time on purpose: where vsync is enforced,
  // FRAME is pinned to the refresh interval and says nothing about the
  // work, while DRAW moves with it.
  float drawAvg = 0.0f;
  if (!_drawHistory.empty()) {
    size_t dFrom = _drawHistory.size() > (size_t)GRAPH_W
                       ? _drawHistory.size() - (size_t)GRAPH_W
                       : 0;
    float  dSum  = 0.0f;
    for (size_t i = dFrom; i < _drawHistory.size(); i++) {
      dSum += _drawHistory[i];
    }
    drawAvg = dSum / (float)(_drawHistory.size() - dFrom);
  }
  std::snprintf(store[0], sizeof(store[0]), "FPS %.1f  FRAME %.2f MS", fps,
                avg);
  std::snprintf(store[1], sizeof(store[1]), "DRAW %.2f MS  WORST %.2f MS",
                drawAvg, _worst);
  std::snprintf(store[2], sizeof(store[2]), "DRAWS %ld  TRIS %ld", draws,
                tris);
  std::snprintf(store[3], sizeof(store[3]), "OBJECTS %ld  CULLED %ld",
                objects, culled);
  std::snprintf(store[4], sizeof(store[4]), "TEXTURES %d", texes);
  std::snprintf(store[5], sizeof(store[5]), "TIME %.2f H",
                OkLighting::getTimeOfDay());
  (void)buf;
  for (int i = 0; i < 6; i++) {
    text[i] = store[i];
    _lines[i]->setText(text[i]);
    // Text is placed by its CENTRE, so a left-aligned column has to
    // shift each line by half its own width -- which changes as the
    // numbers do.
    _lines[i]->setGridPosition(PANEL_X + _lines[i]->getGridWidth() * 0.5f,
                               PANEL_Y - LINE_H * (float)i);
  }

  rebuildGraph();
  // The worst frame decays, so an old hitch stops dominating the panel
  // forever while a recurring one keeps it pinned.
  _worst *= 0.96f;
}

const std::vector<float> &OkGuiStats::getHistory() { return _history; }

void OkGuiStats::setHistoryLength(int samples) {
  _historyMax = samples < 2 ? 2 : samples;
}

/**
 * @brief Min, max, mean and median of the recorded frame times.
 *
 *        The median as well as the mean because they disagree in the
 *        interesting case: a run with occasional long frames has a mean
 *        dragged up by the hitches and a median that ignores them, and
 *        the gap between the two is the hitching itself.
 */
void OkGuiStats::getSummary(int &count, float &minMs, float &maxMs,
                            float &meanMs, float &medianMs) {
  count = (int)_history.size();
  if (count == 0) {
    return;
  }
  std::vector<float> sorted = _history;
  std::sort(sorted.begin(), sorted.end());
  float sum = 0.0f;
  for (size_t i = 0; i < sorted.size(); i++) {
    sum += sorted[i];
  }
  minMs    = sorted.front();
  maxMs    = sorted.back();
  meanMs   = sum / (float)sorted.size();
  medianMs = sorted[sorted.size() / 2];
}

void OkGuiStats::recordDraw(float ms) {
  _drawHistory.push_back(ms);
  while ((int)_drawHistory.size() > _historyMax) {
    _drawHistory.erase(_drawHistory.begin());
  }
}

const std::vector<float> &OkGuiStats::getDrawHistory() {
  return _drawHistory;
}

void OkGuiStats::getDrawSummary(int &count, float &minMs, float &maxMs,
                                float &meanMs, float &medianMs) {
  count = (int)_drawHistory.size();
  if (count == 0) {
    return;
  }
  std::vector<float> sorted = _drawHistory;
  std::sort(sorted.begin(), sorted.end());
  float sum = 0.0f;
  for (size_t i = 0; i < sorted.size(); i++) {
    sum += sorted[i];
  }
  minMs    = sorted.front();
  maxMs    = sorted.back();
  meanMs   = sum / (float)sorted.size();
  medianMs = sorted[sorted.size() / 2];
}

void OkGuiStats::setVisible(bool visible) {
  _visible = visible;
  if (_layer != nullptr) {
    _layer->setVisible(visible);
  }
}

bool OkGuiStats::isVisible() { return _visible; }

void OkGuiStats::shutdown() {
  // The layer owns its elements; the GUI owns the layer.
  _layer    = nullptr;
  _graph    = nullptr;
  _graphTex = nullptr;
  for (int i = 0; i < 6; i++) {
    _lines[i] = nullptr;
  }
  _history.clear();
}
