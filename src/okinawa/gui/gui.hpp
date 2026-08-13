#ifndef OK_GUI_HPP
#define OK_GUI_HPP

#include <string>
#include <vector>

class OkItem;
class OkGuiLayer;

/**
 * @brief Anchor for grid-placed GUI elements: the point of the screen the
 *        grid coordinates are relative to. CENTER is the 0,0 of the grid;
 *        edge and corner anchors keep HUD elements stable across aspect
 *        ratios (a corner element stays a fixed number of cells from ITS
 *        corner on every monitor). Coordinates keep the engine axes
 *        (X+ right, Y+ up), so an element inset from the right edge uses a
 *        negative X offset.
 */
enum OkGuiAnchor {
  OK_GUI_ANCHOR_CENTER = 0,
  OK_GUI_ANCHOR_TOP,
  OK_GUI_ANCHOR_BOTTOM,
  OK_GUI_ANCHOR_LEFT,
  OK_GUI_ANCHOR_RIGHT,
  OK_GUI_ANCHOR_TOP_LEFT,
  OK_GUI_ANCHOR_TOP_RIGHT,
  OK_GUI_ANCHOR_BOTTOM_LEFT,
  OK_GUI_ANCHOR_BOTTOM_RIGHT,
};

/**
 * @brief Static handler for the engine GUI.
 *
 *        The GUI is drawn in a dedicated pass after the 3D scene, using the
 *        SAME shader and item machinery as the rest of the engine: GUI
 *        elements are plain OkItems, only PLACED in a special way.
 *
 *        Placement uses a GRID of cells: origin 0,0 at the screen centre,
 *        X+ right / Y+ up (engine axes), one cell = gui.grid.size logical
 *        pixels (scaled by gui.scale). Every element position and size is
 *        expressed in grid units, so distances across the whole interface
 *        stay multiples of one module.
 *
 *        The pass renders with a fixed PERSPECTIVE camera calibrated so the
 *        Z=0 plane maps 1 world unit to exactly 1 logical pixel: unrotated
 *        elements are pixel-exact on the grid, while rotated ones get true
 *        perspective foreshortening (oblique HUD elements) with the engine's
 *        own math. gui.fov is the aesthetic knob: a larger fov makes oblique
 *        elements converge harder; a tiny fov approaches an orthographic
 *        look.
 */
class OkGui {
public:
  // Delete constructor to prevent instantiation (static handler).
  OkGui() = delete;

  // Create the internal state (reads the gui.* config keys). Called by
  // OkCore::initialize after the GL context exists.
  static void initialize();

  // Destroy the internal items. Called by OkCore::exit.
  static void shutdown();

  // Render the GUI pass: blending on, depth test off, painter's order.
  // Called by OkCore::loop after the scene and camera pass, every frame.
  static void draw();

  // Grid <-> logical-pixel conversions (screen origin at the centre,
  // X+ right, Y+ up). One grid unit = gui.grid.size * scale logical pixels.
  static float gridToScreenX(float gx);
  static float gridToScreenY(float gy);
  static float screenToGridX(float px);
  static float screenToGridY(float py);

  // Effective UI scale: gui.scale when non-zero, otherwise resolved from
  // the monitor content scale so the UI keeps its apparent size on HiDPI.
  static float getScale();

  // Grid cell size in logical pixels, before scaling (gui.grid.size).
  static float getCellSize();

  // Anchor origin in logical pixels for the CURRENT window size, and the
  // pure helpers behind it (testable without a window).
  static float anchorOriginX(OkGuiAnchor anchor);
  static float anchorOriginY(OkGuiAnchor anchor);
  static float anchorOriginXFor(OkGuiAnchor anchor, float logicalW);
  static float anchorOriginYFor(OkGuiAnchor anchor, float logicalH);

  // Layers: named depth layers rendered from the lowest order to the
  // highest (far to near); a higher order paints on top. addLayer returns
  // the new layer (owned by OkGui); getLayer finds it by name (null when
  // missing); removeLayer destroys it and every item it owns.
  static OkGuiLayer *addLayer(const std::string &name, int order);
  static OkGuiLayer *getLayer(const std::string &name);
  static bool        removeLayer(const std::string &name);
  static int         getLayerCount();

  // Show / hide the debug grid overlay (also gui.debug.grid config key).
  static void setDebugGrid(bool show);
  static bool getDebugGrid();

  // Perspective camera calibration for the current window size: distance
  // from the GUI plane such that Z=0 maps 1 unit = 1 logical pixel.
  static float getCameraDistance();

private:
  // Rebuild the debug grid line items if the window size, cell size or
  // scale changed since the last build.
  static void updateDebugGrid(float logicalW, float logicalH);

  // Query the current logical window size (window coordinates).
  static void getLogicalSize(float &outW, float &outH);

  static bool                      _initialized;
  static std::vector<OkGuiLayer *> _layers;     // owned, kept sorted by order
  static OkItem                   *_gridMinor;  // one line per cell (owned)
  static OkItem *_gridMajor;  // stronger line every 5 cells (owned)
  static OkItem *_gridAxes;   // the two 0,0 axes (owned)

  // Cached values the debug grid was last built with.
  static float _builtW;
  static float _builtH;
  static float _builtStep;
};

#endif
