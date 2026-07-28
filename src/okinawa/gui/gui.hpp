#ifndef OK_GUI_HPP
#define OK_GUI_HPP

#include <vector>

class OkItem;

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

  static bool    _initialized;
  static OkItem *_gridMinor;  // one line per cell (owned)
  static OkItem *_gridMajor;  // stronger line every 5 cells (owned)
  static OkItem *_gridAxes;   // the two 0,0 axes (owned)

  // Cached values the debug grid was last built with.
  static float _builtW;
  static float _builtH;
  static float _builtStep;
};

#endif
