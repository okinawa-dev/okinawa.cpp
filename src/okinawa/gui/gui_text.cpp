#include "gui_text.hpp"
#include "../item/item.hpp"
#include "../item/texture.hpp"
#include "font.hpp"
#include <vector>

OkGuiText::OkGuiText(const std::string &name) : OkObject(name) {
  _text     = "";
  _dirty    = false;
  _gridX    = 0.0f;
  _gridY    = 0.0f;
  _gridH    = 1.0f;
  _anchor   = OK_GUI_ANCHOR_CENTER;
  _color[0] = 1.0f;
  _color[1] = 1.0f;
  _color[2] = 1.0f;
  _color[3] = 1.0f;
  _mesh     = nullptr;
}

OkGuiText::~OkGuiText() {
  if (_mesh != nullptr) {
    detachAllChildren();
    delete _mesh;
    _mesh = nullptr;
  }
}

void OkGuiText::setText(const std::string &text) {
  if (_text == text) {
    return;
  }
  _text  = text;
  _dirty = true;
}

void OkGuiText::setGridPosition(float gx, float gy) {
  _gridX = gx;
  _gridY = gy;
}

/**
 * @brief Width of the current text in grid cells: glyph advance scaled by
 *        the cell height (glyphs are GLYPH_H font pixels tall).
 */
float OkGuiText::getGridWidth() const {
  if (_text.empty()) {
    return 0.0f;
  }
  float perChar = (float)OkFont::ADVANCE / (float)OkFont::GLYPH_H;
  return (float)_text.size() * perChar * _gridH;
}

void OkGuiText::setTextColor(float r, float g, float b, float a) {
  _color[0] = r;
  _color[1] = g;
  _color[2] = b;
  _color[3] = a;
  if (_mesh != nullptr) {
    _mesh->setTintColor(r, g, b, a);
  }
}

/**
 * @brief Bake the current text into a standalone RGBA texture (transparent
 *        background, text colour applied) via OkFont::bake.
 */
OkTexture *OkGuiText::bakeTexture(const std::string &textureName,
                                  int                scale) const {
  unsigned char fg[4];
  fg[0]               = (unsigned char)(_color[0] * 255.0f);
  fg[1]               = (unsigned char)(_color[1] * 255.0f);
  fg[2]               = (unsigned char)(_color[2] * 255.0f);
  fg[3]               = (unsigned char)(_color[3] * 255.0f);
  unsigned char bg[4] = {0, 0, 0, 0};
  return OkFont::bake(textureName, _text, scale, fg, bg);
}

/**
 * @brief Rebuild the glyph mesh: one quad per character against the shared
 *        atlas, in LOCAL font-pixel units (origin at the text centre, so
 *        rotations pivot like OkGuiImage). The object scaling maps font
 *        pixels to logical pixels at draw time.
 */
void OkGuiText::rebuildMesh() {
  if (_mesh != nullptr) {
    detachAllChildren();
    delete _mesh;
  }
  _mesh  = nullptr;
  _dirty = false;

  if (_text.empty()) {
    return;
  }

  std::vector<float>        verts;
  std::vector<unsigned int> idx;
  verts.reserve(_text.size() * 20);
  idx.reserve(_text.size() * 6);

  float totalW = (float)_text.size() * (float)OkFont::ADVANCE;
  float halfW  = totalW * 0.5f;
  float halfH  = (float)OkFont::GLYPH_H * 0.5f;

  for (std::size_t i = 0; i < _text.size(); i++) {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    OkFont::glyphUV(_text[i], u0, v0, u1, v1);

    float x0 = (float)i * (float)OkFont::ADVANCE - halfW;
    float x1 = x0 + (float)OkFont::GLYPH_W;

    unsigned int base     = (unsigned int)(verts.size() / 5);
    float        quad[20] = {
        x0, -halfH, 0.0f, u0, v0,  // bottom-left
        x1, -halfH, 0.0f, u1, v0,  // bottom-right
        x1, halfH,  0.0f, u1, v1,  // top-right
        x0, halfH,  0.0f, u0, v1,  // top-left
    };
    for (int k = 0; k < 20; k++) {
      verts.push_back(quad[k]);
    }
    idx.push_back(base);
    idx.push_back(base + 1);
    idx.push_back(base + 2);
    idx.push_back(base);
    idx.push_back(base + 2);
    idx.push_back(base + 3);
  }

  _mesh = new OkItem(getName() + "_mesh", verts.data(), (long)verts.size(),
                     idx.data(), (long)idx.size());
  _mesh->setWireframeGlobal(false);  // text is interface, not scene
  _mesh->setTexture("okfont_atlas", OkFont::atlas());
  _mesh->setTintColor(_color[0], _color[1], _color[2], _color[3]);
  attach(_mesh);
}

/**
 * @brief Sync the transform from the grid (anchor + cells, height mapping
 *        GLYPH_H font pixels to gridHeight cells) and let the child mesh
 *        draw through the hierarchy.
 */
void OkGuiText::drawSelf() {
  if (_dirty) {
    rebuildMesh();
  }
  if (_mesh == nullptr) {
    return;
  }

  setPosition(OkGui::anchorOriginX(_anchor) + OkGui::gridToScreenX(_gridX),
              OkGui::anchorOriginY(_anchor) + OkGui::gridToScreenY(_gridY),
              0.0f);
  float pxPerFontPx = OkGui::gridToScreenY(_gridH) / (float)OkFont::GLYPH_H;
  setScaling(pxPerFontPx, pxPerFontPx, 1.0f);
}
