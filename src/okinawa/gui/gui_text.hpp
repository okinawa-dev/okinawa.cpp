#ifndef OK_GUI_TEXT_HPP
#define OK_GUI_TEXT_HPP

#include "gui.hpp"
#include "../core/object.hpp"
#include <string>

class OkItem;
class OkTexture;

/**
 * @brief Grid-placed text element rendered with the engine bitmap font
 *        (OkFont). setText rebuilds a mesh with ONE QUAD PER CHARACTER
 *        against the shared glyph atlas, so changing text never creates
 *        textures — the right tool for dynamic text (console lines, HUD
 *        counters).
 *
 *        For STATIC text, bakeTexture() rasterizes the current string into
 *        a standalone OkTexture (via OkFont::bake): assign it to an
 *        OkGuiImage or a billboard and drop the OkGuiText.
 *
 *        Position, anchor and HEIGHT are grid units: the glyph height maps
 *        to gridHeight cells and the width follows from the text length
 *        (5x7 glyphs, 1px advance gap). Colour via setTextColor (tints the
 *        white atlas).
 */
class OkGuiText : public OkObject {
public:
  explicit OkGuiText(const std::string &name);
  ~OkGuiText() override;

  // Text content; the mesh is rebuilt lazily on the next draw.
  void               setText(const std::string &text);
  const std::string &getText() const { return _text; }

  // Grid placement (same contract as OkGuiImage).
  void  setGridPosition(float gx, float gy);
  float getGridX() const { return _gridX; }
  float getGridY() const { return _gridY; }

  void        setGridAnchor(OkGuiAnchor anchor) { _anchor = anchor; }
  OkGuiAnchor getGridAnchor() const { return _anchor; }

  // Glyph height in grid cells (default 1 cell). Width is derived.
  void  setGridHeight(float hCells) { _gridH = hCells; }
  float getGridHeight() const { return _gridH; }

  // Width of the current text in grid cells (for layout math).
  float getGridWidth() const;

  // Text colour (tint over the white atlas glyphs).
  void setTextColor(float r, float g, float b, float a);

  // Rasterize the CURRENT text into a standalone texture registered under
  // `textureName` (transparent background, text colour applied). For
  // static text: bake once, assign to an OkGuiImage, drop this element.
  OkTexture *bakeTexture(const std::string &textureName, int scale) const;

  // OkObject hooks: sync transform from the grid and rebuild on demand.
  void drawSelf() override;
  void stepSelf(float dt) override { (void)dt; }
  void updateTransformSelf() override {}

private:
  void rebuildMesh();

  std::string _text;
  bool        _dirty;
  float       _gridX;
  float       _gridY;
  float       _gridH;
  OkGuiAnchor _anchor;
  float       _color[4];
  OkItem     *_mesh;  // owned, recreated on text change
};

#endif
