#include "gui.hpp"
#include "gui_layer.hpp"
#include "../config/config.hpp"
#include "../core/core.hpp"
#include "../item/item.hpp"
#include "../utils/logger.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

bool    OkGui::_initialized = false;
std::vector<OkGuiLayer *> OkGui::_layers;
OkItem *OkGui::_gridMinor   = nullptr;
OkItem *OkGui::_gridMajor   = nullptr;
OkItem *OkGui::_gridAxes    = nullptr;
float   OkGui::_builtW      = 0.0f;
float   OkGui::_builtH      = 0.0f;
float   OkGui::_builtStep   = 0.0f;

// Grid line colours (minor cell lines, stronger every-5 lines, 0,0 axes).
// NOLINTBEGIN(readability-magic-numbers)
static const float GRID_MINOR_COLOR[3] = {0.25f, 0.30f, 0.35f};
static const float GRID_MAJOR_COLOR[3] = {0.40f, 0.48f, 0.55f};
static const float GRID_AXES_COLOR[3]  = {0.70f, 0.80f, 0.30f};
// NOLINTEND(readability-magic-numbers)

// Every how many cells the stronger (major) line is drawn.
static const int GRID_MAJOR_EVERY = 5;

/**
 * @brief Create the GUI state. Reads the gui.* config keys; the debug grid
 *        items themselves are built lazily on the first draw (they depend on
 *        the window size).
 */
void OkGui::initialize() {
  _initialized = true;
  OkLogger::info("Gui", "GUI initialized (cell " +
                            std::to_string(OkConfig::getInt("gui.grid.size")) +
                            " logical px)");
}

/**
 * @brief Destroy the internal debug grid items.
 */
void OkGui::shutdown() {
  for (std::size_t i = 0; i < _layers.size(); i++) {
    delete _layers[i];
  }
  _layers.clear();
  delete _gridMinor;
  delete _gridMajor;
  delete _gridAxes;
  _gridMinor   = nullptr;
  _gridMajor   = nullptr;
  _gridAxes    = nullptr;
  _initialized = false;
}

/**
 * @brief Current logical window size in window coordinates. On macOS these
 *        are points (already density-independent); on platforms where the
 *        window is reported in physical pixels, getScale() compensates.
 */
void OkGui::getLogicalSize(float &outW, float &outH) {
  GLFWwindow *window = OkCore::getWindow();
  int         w      = 0;
  int         h      = 0;
  if (window != nullptr) {
    glfwGetWindowSize(window, &w, &h);
  }
  outW = (float)w;
  outH = (float)h;
}

/**
 * @brief Effective UI scale. gui.scale when non-zero; otherwise resolved as
 *        contentScale * (window / framebuffer), which is 1.0 on macOS retina
 *        (window coords are already logical points) and the monitor content
 *        scale on platforms whose window coords are physical pixels.
 */
float OkGui::getScale() {
  float configured = OkConfig::getFloat("gui.scale");
  if (configured > 0.0f) {
    return configured;
  }

  GLFWwindow *window = OkCore::getWindow();
  if (window == nullptr) {
    return 1.0f;
  }

  float csx = 1.0f;
  float csy = 1.0f;
  glfwGetWindowContentScale(window, &csx, &csy);

  int ww = 0;
  int wh = 0;
  int fw = 0;
  int fh = 0;
  glfwGetWindowSize(window, &ww, &wh);
  glfwGetFramebufferSize(window, &fw, &fh);
  if (fw <= 0 || ww <= 0) {
    return csx;
  }

  return csx * ((float)ww / (float)fw);
}

/**
 * @brief Grid cell size in logical pixels, before scaling.
 */
float OkGui::getCellSize() { return (float)OkConfig::getInt("gui.grid.size"); }

float OkGui::gridToScreenX(float gx) { return gx * getCellSize() * getScale(); }
float OkGui::gridToScreenY(float gy) { return gy * getCellSize() * getScale(); }

float OkGui::screenToGridX(float px) {
  float step = getCellSize() * getScale();
  return (step > 0.0f) ? (px / step) : 0.0f;
}

float OkGui::screenToGridY(float py) {
  float step = getCellSize() * getScale();
  return (step > 0.0f) ? (py / step) : 0.0f;
}

/**
 * @brief Distance of the calibrated GUI camera from the Z=0 plane for the
 *        current window: D = (logicalHeight / 2) / tan(fov / 2), so that at
 *        Z=0 one world unit projects to exactly one logical pixel.
 */
float OkGui::getCameraDistance() {
  float logicalW = 0.0f;
  float logicalH = 0.0f;
  getLogicalSize(logicalW, logicalH);

  float fovDeg = OkConfig::getFloat("gui.fov");
  float fovRad = glm::radians(fovDeg);
  float tanH   = std::tan(fovRad * 0.5f);
  if (tanH <= 0.0f) {
    return 1.0f;
  }
  return (logicalH * 0.5f) / tanH;
}

/**
 * @brief Anchor origin helpers: X of the anchor point for a window of the
 *        given logical width (centre-origin coordinates, X+ right).
 */
float OkGui::anchorOriginXFor(OkGuiAnchor anchor, float logicalW) {
  switch (anchor) {
  case OK_GUI_ANCHOR_LEFT:
  case OK_GUI_ANCHOR_TOP_LEFT:
  case OK_GUI_ANCHOR_BOTTOM_LEFT:
    return -logicalW * 0.5f;
  case OK_GUI_ANCHOR_RIGHT:
  case OK_GUI_ANCHOR_TOP_RIGHT:
  case OK_GUI_ANCHOR_BOTTOM_RIGHT:
    return logicalW * 0.5f;
  default:
    return 0.0f;
  }
}

/**
 * @brief Anchor origin helpers: Y of the anchor point for a window of the
 *        given logical height (centre-origin coordinates, Y+ up).
 */
float OkGui::anchorOriginYFor(OkGuiAnchor anchor, float logicalH) {
  switch (anchor) {
  case OK_GUI_ANCHOR_TOP:
  case OK_GUI_ANCHOR_TOP_LEFT:
  case OK_GUI_ANCHOR_TOP_RIGHT:
    return logicalH * 0.5f;
  case OK_GUI_ANCHOR_BOTTOM:
  case OK_GUI_ANCHOR_BOTTOM_LEFT:
  case OK_GUI_ANCHOR_BOTTOM_RIGHT:
    return -logicalH * 0.5f;
  default:
    return 0.0f;
  }
}

float OkGui::anchorOriginX(OkGuiAnchor anchor) {
  float logicalW = 0.0f;
  float logicalH = 0.0f;
  getLogicalSize(logicalW, logicalH);
  return anchorOriginXFor(anchor, logicalW);
}

float OkGui::anchorOriginY(OkGuiAnchor anchor) {
  float logicalW = 0.0f;
  float logicalH = 0.0f;
  getLogicalSize(logicalW, logicalH);
  return anchorOriginYFor(anchor, logicalH);
}

/**
 * @brief Create a named layer (owned by OkGui) and keep the list sorted by
 *        order so the draw pass walks it far-to-near without re-sorting.
 */
OkGuiLayer *OkGui::addLayer(const std::string &name, int order) {
  OkGuiLayer *layer = new OkGuiLayer(name, order);
  std::size_t at    = _layers.size();
  for (std::size_t i = 0; i < _layers.size(); i++) {
    if (_layers[i]->getOrder() > order) {
      at = i;
      break;
    }
  }
  _layers.insert(_layers.begin() + (long)at, layer);
  return layer;
}

OkGuiLayer *OkGui::getLayer(const std::string &name) {
  for (std::size_t i = 0; i < _layers.size(); i++) {
    if (_layers[i]->getName() == name) {
      return _layers[i];
    }
  }
  return nullptr;
}

/**
 * @brief Destroy a layer and every item it owns. True if it existed.
 */
bool OkGui::removeLayer(const std::string &name) {
  for (std::size_t i = 0; i < _layers.size(); i++) {
    if (_layers[i]->getName() == name) {
      delete _layers[i];
      _layers.erase(_layers.begin() + (long)i);
      return true;
    }
  }
  return false;
}

int OkGui::getLayerCount() { return (int)_layers.size(); }

void OkGui::setDebugGrid(bool show) {
  OkConfig::setBool("gui.debug.grid", show);
}

bool OkGui::getDebugGrid() { return OkConfig::getBool("gui.debug.grid"); }

/**
 * @brief Build (or rebuild) the three debug grid line items for the given
 *        logical window size: one item per line class so each keeps its own
 *        colour (minor cell lines, stronger every-5 lines, centre axes).
 */
void OkGui::updateDebugGrid(float logicalW, float logicalH) {
  float step = getCellSize() * getScale();
  if (step <= 0.0f) {
    return;
  }
  if (logicalW == _builtW && logicalH == _builtH && step == _builtStep) {
    return;
  }

  delete _gridMinor;
  delete _gridMajor;
  delete _gridAxes;
  _gridMinor = nullptr;
  _gridMajor = nullptr;
  _gridAxes  = nullptr;

  float halfW = logicalW * 0.5f;
  float halfH = logicalH * 0.5f;

  // Vertex stride is 5 floats (position + dummy UV), same as OkItem expects.
  std::vector<float>        minorVerts;
  std::vector<unsigned int> minorIdx;
  std::vector<float>        majorVerts;
  std::vector<unsigned int> majorIdx;
  std::vector<float>        axesVerts;
  std::vector<unsigned int> axesIdx;

  struct Push {
    static void line(std::vector<float> &verts, std::vector<unsigned int> &idx,
                     float x0, float y0, float x1, float y1) {
      unsigned int base = (unsigned int)(verts.size() / 5);
      verts.push_back(x0);
      verts.push_back(y0);
      verts.push_back(0.0f);
      verts.push_back(0.0f);
      verts.push_back(0.0f);
      verts.push_back(x1);
      verts.push_back(y1);
      verts.push_back(0.0f);
      verts.push_back(0.0f);
      verts.push_back(0.0f);
      idx.push_back(base);
      idx.push_back(base + 1);
    }
  };

  // Vertical lines every cell from the centre outward, symmetric.
  int cellsX = (int)std::ceil(halfW / step);
  for (int k = -cellsX; k <= cellsX; k++) {
    float x = (float)k * step;
    if (k == 0) {
      Push::line(axesVerts, axesIdx, x, -halfH, x, halfH);
    } else if ((k % GRID_MAJOR_EVERY) == 0) {
      Push::line(majorVerts, majorIdx, x, -halfH, x, halfH);
    } else {
      Push::line(minorVerts, minorIdx, x, -halfH, x, halfH);
    }
  }

  // Horizontal lines every cell from the centre outward, symmetric.
  int cellsY = (int)std::ceil(halfH / step);
  for (int k = -cellsY; k <= cellsY; k++) {
    float y = (float)k * step;
    if (k == 0) {
      Push::line(axesVerts, axesIdx, -halfW, y, halfW, y);
    } else if ((k % GRID_MAJOR_EVERY) == 0) {
      Push::line(majorVerts, majorIdx, -halfW, y, halfW, y);
    } else {
      Push::line(minorVerts, minorIdx, -halfW, y, halfW, y);
    }
  }

  if (!minorIdx.empty()) {
    _gridMinor = new OkItem("gui_grid_minor", minorVerts.data(),
                            (long)minorVerts.size(), minorIdx.data(),
                            (long)minorIdx.size());
    _gridMinor->setDrawMode(GL_LINES);
    _gridMinor->setFillColor(GRID_MINOR_COLOR[0], GRID_MINOR_COLOR[1],
                             GRID_MINOR_COLOR[2]);
  }
  if (!majorIdx.empty()) {
    _gridMajor = new OkItem("gui_grid_major", majorVerts.data(),
                            (long)majorVerts.size(), majorIdx.data(),
                            (long)majorIdx.size());
    _gridMajor->setDrawMode(GL_LINES);
    _gridMajor->setFillColor(GRID_MAJOR_COLOR[0], GRID_MAJOR_COLOR[1],
                             GRID_MAJOR_COLOR[2]);
  }
  if (!axesIdx.empty()) {
    _gridAxes = new OkItem("gui_grid_axes", axesVerts.data(),
                           (long)axesVerts.size(), axesIdx.data(),
                           (long)axesIdx.size());
    _gridAxes->setDrawMode(GL_LINES);
    _gridAxes->setFillColor(GRID_AXES_COLOR[0], GRID_AXES_COLOR[1],
                            GRID_AXES_COLOR[2]);
  }

  _builtW    = logicalW;
  _builtH    = logicalH;
  _builtStep = step;
}

/**
 * @brief Render the GUI pass. Uses the engine shader already in use for the
 *        frame; only the view/projection uniforms are swapped for the
 *        calibrated GUI camera, depth testing is disabled (painter's order)
 *        and blending enabled. Runs after the scene and camera passes.
 */
void OkGui::draw() {
  if (!_initialized) {
    return;
  }

  float logicalW = 0.0f;
  float logicalH = 0.0f;
  getLogicalSize(logicalW, logicalH);
  if (logicalW <= 0.0f || logicalH <= 0.0f) {
    return;
  }

  bool showGrid = getDebugGrid();
  if (!showGrid && _layers.empty()) {
    return;
  }

  if (showGrid) {
    updateDebugGrid(logicalW, logicalH);
  }

  GLuint program = OkCore::getShaderProgram();
  glUseProgram(program);

  // Calibrated perspective camera: at Z=0 one unit == one logical pixel.
  float     dist   = getCameraDistance();
  float     fovDeg = OkConfig::getFloat("gui.fov");
  float     aspect = logicalW / logicalH;
  glm::mat4 projection =
      glm::perspective(glm::radians(fovDeg), aspect, dist * 0.1f, dist * 10.0f);
  glm::mat4 view =
      glm::lookAt(glm::vec3(0.0f, 0.0f, dist), glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 1.0f, 0.0f));

  GLint viewLoc = glGetUniformLocation(program, "view");
  GLint projLoc = glGetUniformLocation(program, "projection");
  if (viewLoc != -1) {
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
  }
  if (projLoc != -1) {
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
  }

  // The interface lives outside the world's atmosphere: no tint, no fog,
  // no sun (lightingOn 0 makes the Gouraud stage a neutral 1).
  {
    GLint tintLoc   = glGetUniformLocation(program, "sceneTint");
    GLint fogDenLoc = glGetUniformLocation(program, "fogDensity");
    GLint litLoc    = glGetUniformLocation(program, "lightingOn");
    if (tintLoc != -1) {
      glUniform3f(tintLoc, 1.0f, 1.0f, 1.0f);
    }
    if (fogDenLoc != -1) {
      glUniform1f(fogDenLoc, 0.0f);
    }
    if (litLoc != -1) {
      glUniform1f(litLoc, 0.0f);
    }
  }

  // Painter's order: depth is the layer list, not the Z buffer.
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Layers, far to near (the list is kept sorted by order).
  for (std::size_t i = 0; i < _layers.size(); i++) {
    _layers[i]->draw();
  }

  // The authoring grid draws on top of everything.
  if (showGrid) {
    if (_gridMinor != nullptr) {
      _gridMinor->draw();
    }
    if (_gridMajor != nullptr) {
      _gridMajor->draw();
    }
    if (_gridAxes != nullptr) {
      _gridAxes->draw();
    }
  }

  glEnable(GL_DEPTH_TEST);
}
