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

  static void shutdown();

private:
  static void rebuildGraph();

  static OkGuiLayer *_layer;
  static OkGuiText  *_lines[6];
  static OkGuiImage *_graph;
  static OkTexture  *_graphTex;

  static std::vector<float> _history;   // frame times, milliseconds
  static float              _accum;     // time since the last refresh
  static float              _worst;     // worst frame in the window
  static bool               _visible;
};

#endif
