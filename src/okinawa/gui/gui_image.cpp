#include "gui_image.hpp"
#include "gui.hpp"

// Unit quad centred on the origin, on the Z=0 plane. Vertex stride is
// 5 floats (position + UV); v=0 at the bottom (textures load flipped for
// GL, so this shows the image upright).
// NOLINTBEGIN(readability-magic-numbers)
static float GUI_QUAD_VERTS[20] = {
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  // bottom-left
    0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,  // bottom-right
    0.5f,  0.5f,  0.0f, 1.0f, 1.0f,  // top-right
    -0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  // top-left
};
static unsigned int GUI_QUAD_INDICES[6] = {0, 1, 2, 0, 2, 3};
// NOLINTEND(readability-magic-numbers)

OkGuiImage::OkGuiImage(const std::string &name)
    : OkItem(name, GUI_QUAD_VERTS, 20, GUI_QUAD_INDICES, 6) {
  _gridX  = 0.0f;
  _gridY  = 0.0f;
  _gridW  = 1.0f;
  _gridH  = 1.0f;
  _anchor = OK_GUI_ANCHOR_CENTER;
  // The interface is drawn over the scene, not part of it: the global
  // wireframe switch is a way of reading the world's geometry and has
  // nothing to say about a GUI quad.
  setWireframeGlobal(false);
}

/**
 * @brief Place the element centre on the grid (cells from the screen centre).
 */
void OkGuiImage::setGridPosition(float gx, float gy) {
  _gridX = gx;
  _gridY = gy;
}

/**
 * @brief Size the quad in grid cells.
 */
void OkGuiImage::setGridSize(float wCells, float hCells) {
  _gridW = wCells;
  _gridH = hCells;
}

/**
 * @brief Sync the world transform from the grid coordinates and draw. The
 *        conversion runs every draw because the effective cell size in
 *        logical pixels can change with the window or monitor; rotation is
 *        left untouched (it belongs to the caller).
 */
void OkGuiImage::drawSelf() {
  setPosition(OkGui::anchorOriginX(_anchor) + OkGui::gridToScreenX(_gridX),
              OkGui::anchorOriginY(_anchor) + OkGui::gridToScreenY(_gridY),
              0.0f);
  setScaling(OkGui::gridToScreenX(_gridW), OkGui::gridToScreenY(_gridH), 1.0f);
  OkItem::drawSelf();
}
