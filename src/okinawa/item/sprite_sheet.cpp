#include "sprite_sheet.hpp"
#include "../handlers/textures.hpp"
#include "../utils/files.hpp"
#include "../utils/logger.hpp"
#include "texture.hpp"
#include <nlohmann/json.hpp>

using nlohmann::json;

OkSpriteSheet::OkSpriteSheet() {
  _texture = nullptr;
  _width   = 0;
  _height  = 0;
}

OkSpriteSheet::~OkSpriteSheet() {
  if (_texture != nullptr && !_textureName.empty()) {
    OkTextureHandler::getInstance()->removeReference(_textureName);
    _texture = nullptr;
  }
}

namespace {

  // The directory part of a path, so meta.image resolves next to the JSON.
  std::string dirOf(const std::string &path) {
    size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
      return "";
    }
    return path.substr(0, slash + 1);
  }

}  // namespace

/**
 * @brief Read the sheet description and upload its image once.
 *
 *        The dialect is Aseprite's (TexturePacker's is the same core):
 *        "frames" may be a hash keyed by name or an array of objects
 *        with "filename"; each carries frame{x,y,w,h}, and optionally
 *        rotated, trimmed and duration. "meta" carries image, size and
 *        frameTags. Everything is optional except the rectangles -- a
 *        minimal hand-written file works.
 */
bool OkSpriteSheet::load(const std::string &jsonPath,
                         const std::string &imageOverride) {
  std::string text = OkFiles::readFile(jsonPath);
  if (text.empty()) {
    OkLogger::error("SpriteSheet", "Cannot read " + jsonPath);
    return false;
  }

  json doc;
  try {
    doc = json::parse(text);
  } catch (const std::exception &e) {
    OkLogger::error("SpriteSheet",
                    "Malformed JSON in " + jsonPath + ": " + e.what());
    return false;
  }

  // Sheet size: from meta when present, otherwise from the image itself
  // after loading (regions need it to compute UVs).
  _width            = 0;
  _height           = 0;
  std::string image = imageOverride;
  if (doc.contains("meta")) {
    const json &meta = doc["meta"];
    if (image.empty() && meta.contains("image") && meta["image"].is_string()) {
      image = dirOf(jsonPath) + meta["image"].get<std::string>();
    }
    if (meta.contains("size")) {
      _width  = meta["size"].value("w", 0);
      _height = meta["size"].value("h", 0);
    }
  }
  if (image.empty()) {
    OkLogger::error("SpriteSheet", "No image named by " + jsonPath);
    return false;
  }

  _texture = OkTextureHandler::getInstance()->createTextureFromFile(image);
  if (_texture == nullptr) {
    OkLogger::error("SpriteSheet", "Cannot load image " + image);
    return false;
  }
  _textureName = image;
  if (_width <= 0 || _height <= 0) {
    _width  = _texture->getWidth();
    _height = _texture->getHeight();
  }
  if (_width <= 0 || _height <= 0) {
    OkLogger::error("SpriteSheet", "Unknown sheet size for " + jsonPath);
    return false;
  }

  // Regions. Accept both the hash and the array shape.
  _regions.clear();
  _order.clear();
  if (doc.contains("frames")) {
    const json                                       &frames = doc["frames"];
    std::vector<std::pair<std::string, const json *>> entries;
    if (frames.is_object()) {
      for (json::const_iterator it = frames.begin(); it != frames.end(); ++it) {
        entries.emplace_back(it.key(), &it.value());
      }
    } else if (frames.is_array()) {
      for (size_t i = 0; i < frames.size(); i++) {
        std::string name = frames[i].value("filename", "");
        if (name.empty()) {
          name = "region_" + std::to_string(i);
        }
        entries.emplace_back(name, &frames[i]);
      }
    }

    for (size_t i = 0; i < entries.size(); i++) {
      const json &e = *entries[i].second;
      if (!e.contains("frame")) {
        continue;
      }
      const json    &r = e["frame"];
      OkSpriteRegion region;
      region.name       = entries[i].first;
      region.x          = r.value("x", 0);
      region.y          = r.value("y", 0);
      region.w          = r.value("w", 0);
      region.h          = r.value("h", 0);
      region.rotated    = e.value("rotated", false);
      region.durationMs = e.value("duration", 0);
      if (region.w <= 0 || region.h <= 0) {
        continue;
      }
      // Textures load flipped for GL, so the region's TOP pixel row maps
      // to the higher v. v0 is the bottom of the region.
      float fw  = static_cast<float>(_width);
      float fh  = static_cast<float>(_height);
      region.u0 = static_cast<float>(region.x) / fw;
      region.u1 = static_cast<float>(region.x + region.w) / fw;
      region.v1 = 1.0f - static_cast<float>(region.y) / fh;
      region.v0 = 1.0f - static_cast<float>(region.y + region.h) / fh;
      _regions[region.name] = region;
      _order.push_back(region.name);
    }
  }

  // Groups from the sheet's tags. Aseprite writes frameTags as ranges of
  // frame INDICES; we keep the names in that range, which lets a caller
  // ask for a family of pieces ("window_tall") without knowing them.
  _groups.clear();
  if (doc.contains("meta") && doc["meta"].contains("frameTags")) {
    const json &tags = doc["meta"]["frameTags"];
    for (size_t i = 0; i < tags.size(); i++) {
      std::string name = tags[i].value("name", "");
      int         from = tags[i].value("from", 0);
      int         to   = tags[i].value("to", 0);
      if (name.empty()) {
        continue;
      }
      std::vector<std::string> members;
      for (int k = from; k <= to && k < static_cast<int>(_order.size()); k++) {
        if (k >= 0) {
          members.push_back(_order[static_cast<size_t>(k)]);
        }
      }
      _groups[name] = members;
    }
  }

  OkLogger::info("SpriteSheet", "Loaded " + std::to_string(_regions.size()) +
                                    " region(s) from " + jsonPath + " over " +
                                    image);
  return true;
}

const OkSpriteRegion *OkSpriteSheet::getRegion(const std::string &name) const {
  std::map<std::string, OkSpriteRegion>::const_iterator it =
      _regions.find(name);
  if (it == _regions.end()) {
    return nullptr;
  }
  return &it->second;
}

bool OkSpriteSheet::hasRegion(const std::string &name) const {
  return _regions.find(name) != _regions.end();
}

std::vector<std::string> OkSpriteSheet::getRegionNames() const {
  return _order;
}

int OkSpriteSheet::getRegionCount() const {
  return static_cast<int>(_regions.size());
}

std::vector<std::string> OkSpriteSheet::getGroup(const std::string &tag) const {
  std::map<std::string, std::vector<std::string>>::const_iterator it =
      _groups.find(tag);
  if (it == _groups.end()) {
    return std::vector<std::string>();
  }
  return it->second;
}
