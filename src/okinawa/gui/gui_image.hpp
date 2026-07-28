#ifndef OK_GUI_IMAGE_HPP
#define OK_GUI_IMAGE_HPP

#include "../item/item.hpp"
#include "gui.hpp"
#include <string>

/**
 * @brief A textured quad placed on the GUI grid. It IS a plain OkItem (same
 *        textures, rotation, visibility, wireframe debug); the only new
 *        surface is that position and size are expressed in GRID units
 *        (cells), converted to logical pixels by OkGui just before drawing.
 *
 *        The quad is a unit square centred on its origin, so rotations pivot
 *        around the element centre and grid size maps to the item scaling.
 *        Rotate it with the inherited setRotation for oblique HUD elements:
 *        the calibrated GUI camera gives true perspective foreshortening.
 */
class OkGuiImage : public OkItem {
public:
  explicit OkGuiImage(const std::string &name);

  // Position of the element CENTRE on the grid (cells from the anchor
  // point, X+ right, Y+ up). The anchor defaults to the screen centre;
  // edge/corner anchors keep the element stable across aspect ratios.
  void  setGridPosition(float gx, float gy);
  float getGridX() const { return _gridX; }
  float getGridY() const { return _gridY; }

  void        setGridAnchor(OkGuiAnchor anchor) { _anchor = anchor; }
  OkGuiAnchor getGridAnchor() const { return _anchor; }

  // Size of the quad in grid cells.
  void  setGridSize(float wCells, float hCells);
  float getGridWidth() const { return _gridW; }
  float getGridHeight() const { return _gridH; }

  // OkItem hook: sync the world transform from the grid coordinates (the
  // effective scale can change with the window/monitor), then draw.
  void drawSelf() override;

private:
  float       _gridX;
  float       _gridY;
  float       _gridW;
  float       _gridH;
  OkGuiAnchor _anchor;
};

#endif
