#ifndef OK_FONT_HPP
#define OK_FONT_HPP

#include <string>

class OkTexture;

/**
 * @brief The engine's built-in bitmap font: 5x7 pixel glyphs for printable
 *        ASCII (lowercase maps to uppercase, console style). Two ways to
 *        use it:
 *
 *        - bake(): rasterize a string on the CPU into an RGBA OkTexture
 *          (white glyphs, transparent or custom background). The right
 *          tool for STATIC text: bake once, hand the texture to any
 *          element (an OkGuiImage, a billboard) and forget the font.
 *        - atlas() + glyphUV(): one shared texture with every glyph, for
 *          elements that rebuild text often (OkGuiText emits one quad per
 *          character against it, so changing text never creates textures).
 */
class OkFont {
public:
  OkFont() = delete;

  // Glyph cell size in font pixels.
  static const int GLYPH_W = 5;
  static const int GLYPH_H = 7;
  // Horizontal advance (glyph + 1px gap).
  static const int ADVANCE = 6;

  // The 7 row bitmasks (bit 4 = left column) for a character. Lowercase
  // maps to uppercase; anything without a glyph returns a blank.
  static const unsigned char *glyphRows(char c);

  // Rasterize `text` into an RGBA texture registered under `name` in the
  // texture handler. `scale` is texture pixels per font pixel. fg/bg are
  // RGBA 0..255; a bg alpha of 0 gives transparent text for the blended
  // GUI pass (use an opaque bg for unblended contexts like billboards).
  static OkTexture *bake(const std::string &name, const std::string &text,
                         int scale, const unsigned char *fg,
                         const unsigned char *bg);

  // The shared glyph atlas (lazy, cached in the texture handler): a
  // 16x6-cell grid holding ASCII 32..126, white on transparent.
  static OkTexture *atlas();

  // UV rectangle of a character inside atlas() (u0,v0 = bottom-left).
  static void glyphUV(char c, float &u0, float &v0, float &u1, float &v1);

private:
  // Atlas grid layout (16 columns x 6 rows of glyph cells).
  static const int ATLAS_COLS = 16;
  static const int ATLAS_ROWS = 6;
};

#endif
