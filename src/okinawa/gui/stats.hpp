#ifndef OK_GUI_STATS_HPP
#define OK_GUI_STATS_HPP

#include <string>
#include <vector>

class OkGuiLayer;
class OkGuiText;
class OkGuiImage;
class OkTexture;

/**
 * @brief A small runtime statistics panel, in the spirit of the debug
 *        overlays engines ship with.
 *
 *        Numbers plus a live frame-time graph, anchored to a screen
 *        corner so it survives resizes. It reads what the engine
 *        already tracks (frame timing, scene contents, culling and
 *        render counters, textures, the day clock) and adds nothing to
 *        the frame it is measuring beyond its own handful of quads.
 *
 *        The frame-time GRAPH matters more than the average: a number
 *        hides the hitches, a history shows them. The panel keeps the
 *        last few seconds and draws them as a strip, with the worst
 *        recent frame called out.
 *
 *        Off by default; a project shows it with setVisible(true) or
 *        through the console command it registers ("stats").
 */
class OkGuiStats {
public:
  OkGuiStats() = delete;

  // Build the panel and register its console command. Called by
  // OkCore::initialize, after the GUI and the console.
  static void initialize();

  // Refresh the readings. Called once per frame by OkCore with the
  // frame time in milliseconds.
  static void update(float dtMs);

  static void setVisible(bool visible);
  static bool isVisible();

  // The frame-time history, oldest first, in milliseconds. Kept whether
  // or not the panel is shown, and long enough to cover several seconds
  // -- a single instantaneous reading says nothing about how a build
  // performs, and comparing two of them says less.
  static const std::vector<float> &getHistory();
  // Summary of that history. `count` is 0 when nothing has been
  // recorded yet, in which case the other outputs are left alone.
  static void getSummary(int &count, float &minMs, float &maxMs,
                         float &meanMs, float &medianMs);
  // How many samples to keep. The graph draws its own shorter window.
  static void setHistoryLength(int samples);

  static void shutdown();

private:
  static void rebuildGraph();

  static OkGuiLayer *_layer;
  static OkGuiText  *_lines[6];
  static OkGuiImage *_graph;
  static OkTexture  *_graphTex;

  static std::vector<float> _history;   // frame times, milliseconds
  static int                _historyMax;  // samples kept
  static float              _accum;     // time since the last refresh
  static float              _worst;     // worst frame in the window
  static bool               _visible;
};

#endif
