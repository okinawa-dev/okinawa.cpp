#ifndef OK_SPRITE_SHEET_HPP
#define OK_SPRITE_SHEET_HPP

#include <map>
#include <string>
#include <vector>

class OkTexture;

/**
 * @brief A named region inside a sprite sheet.
 *
 *        Coordinates come in two flavours: the pixel rectangle as
 *        authored, and the UV rectangle ready for a quad. UVs already
 *        account for the engine's convention that textures load flipped
 *        for GL, so v0 is the BOTTOM of the region.
 */
struct OkSpriteRegion {
  std::string name;
  int         x, y, w, h;      // pixels in the sheet, origin top-left
  float       u0, v0, u1, v1;  // ready to hand to a quad
  bool        rotated;         // packed rotated 90 degrees (rare)
  int         durationMs;      // animation hint, 0 when not applicable
};

/**
 * @brief A single texture holding many named regions.
 *
 *        ONE texture is uploaded to the GPU (through the texture handler,
 *        so it is refcounted and shared like any other) and the regions
 *        are metadata: rectangles with a name. That is the whole point of
 *        an atlas -- one upload, one bind, and the chance to draw a
 *        thousand different pieces in a single call. A region is never a
 *        texture of its own.
 *
 *        The description file is read in the ASEPRITE / TexturePacker
 *        JSON dialect, because that is what the pixel-art and packing
 *        tools of the world emit: an artist can redraw the sheet in
 *        Aseprite, export, and the game picks it up with no code change.
 *        Only the parser speaks that dialect; the engine API never says
 *        "frame" -- pieces are REGIONS here, and frames will mean frames
 *        when actual animation arrives (the parser already keeps each
 *        region's duration and the sheet's tags for that day).
 */
class OkSpriteSheet {
public:
  OkSpriteSheet();
  ~OkSpriteSheet();

  // Load `jsonPath` (Aseprite/TexturePacker JSON) plus the image it
  // names. `imageOverride` wins over the JSON's meta.image when given.
  // Returns false and logs on failure.
  bool load(const std::string &jsonPath, const std::string &imageOverride = "");

  // The shared GPU texture. Hand it to items with setTexture().
  [[nodiscard]] OkTexture         *getTexture() const { return _texture; }
  [[nodiscard]] const std::string &getTextureName() const {
    return _textureName;
  }

  // Region lookup by name; null when missing.
  [[nodiscard]] const OkSpriteRegion *getRegion(const std::string &name) const;
  [[nodiscard]] bool                  hasRegion(const std::string &name) const;
  [[nodiscard]] std::vector<std::string> getRegionNames() const;
  [[nodiscard]] int                      getRegionCount() const;

  // Named region groups from the sheet's tags (Aseprite frameTags): the
  // regions whose name falls in a tag's range, in order. Useful both for
  // animation later and to pick a random member of a family today
  // ("window_tall" -> every tall-window variant).
  [[nodiscard]] std::vector<std::string> getGroup(const std::string &tag) const;

  [[nodiscard]] int getWidth() const { return _width; }
  [[nodiscard]] int getHeight() const { return _height; }

private:
  OkTexture  *_texture;
  std::string _textureName;
  int         _width, _height;

  std::map<std::string, OkSpriteRegion>           _regions;
  std::vector<std::string>                        _order;
  std::map<std::string, std::vector<std::string>> _groups;
};

#endif
