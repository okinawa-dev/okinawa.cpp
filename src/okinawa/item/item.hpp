#ifndef OK_ITEM_HPP
#define OK_ITEM_HPP

#include "../core/gl_config.hpp"
#include "../core/object.hpp"
#include "../handlers/textures.hpp"
#include "../item/texture.hpp"
#include <algorithm>
#include <string>
#include <vector>

class OkItem : public OkObject {
private:
  void _initBuffers();

protected:
  // Mesh, material and GL state. Protected rather than private because
  // subclasses issue their own draws with the same state: OkBillboard
  // modulates the tint per frame, OkInstancedItem replaces the draw call
  // with an instanced one.
  bool visible;
  bool drawWireframe;  // Flag to control wireframe rendering
  // Whether the global `graphics.wireframe` switch reaches this item.
  // It is a way of looking at the scene, so the interface drawn over
  // the scene stays out of it: a wireframed font atlas is a screenful
  // of empty boxes, and the console that turns the switch back off is
  // the first thing to become unreadable.
  bool wireframeGlobal;
  // Whether this item is drawn into the shadow maps. Most things are;
  // the ones that are not are the ones that are light rather than
  // matter -- a lamp's corona, an emissive pane, the sky itself. They
  // have geometry, so without this they are recorded as occluders and
  // a glow ends up casting a shadow.
  bool   castsShadow;
  GLenum drawMode;  // GL_TRIANGLES, GL_LINES, etc.

  // Flat fill colour when untextured, and wireframe line colour (RGB, white).
  float fillColor[4];  // RGBA: alpha honoured by blended passes (GUI)
  float wireframeColor[3];

  // Geometry
  float        *vertices;
  unsigned int *indices;
  long          numVertices;
  long          numIndices;
  float         radius;           // bounding-sphere radius (half bbox diagonal)
  float         sphereCenter[3];  // bounding-sphere centre, local coords
  bool          additive;         // additive blending (light halos)
  bool          unlit;            // skip Gouraud light and the scene tint
  int           nearLights[8];    // cached nearest point-light indices
  int           nearLightCount;
  long          nearLightGen;  // registry generation of the cache

  // OpenGL objects
  GLuint VAO, VBO, EBO;

  // Texture
  std::string textureName;  // Name/path of the texture for reference counting

  // Material slots. A mesh often wants more than one material -- a crate
  // with a metal lid, a wall and its roof, a body and its glass -- and
  // splitting it into separate items to get them is the wrong trade:
  // it doubles
  // the objects, the transforms and the culling tests for what is one
  // surface. Instead the index buffer is carved into ranges, each with
  // its own texture, drawn back to back from the same buffers. This is
  // the usual arrangement (submeshes, material slots, glTF primitives).
  //
  // Empty means the whole mesh wears `texture`, which is the common
  // case and costs nothing.
  struct MaterialRange {
    long        first;    // first index into the element buffer
    long        count;    // how many indices
    OkTexture  *texture;  // null draws the range in the fill colour
    std::string textureName;
  };
  std::vector<MaterialRange> materials;
  OkTexture                 *texture;

  // Multiplies the texture in the fill pass.
  float tintColor[4];
  bool  maskedMaterials;
  float fade;          // 1 = solid; below that, dithered away
  bool  fadeInverted;  // use the opposite half of the pattern
  float matTint[3][3];
  float matLuma[3];

  // Geometry
  void _calculateRadius();

  // Store vertex data with the internal stride-8 layout; stride-5 input
  // gets normals computed from the triangle list (see item.cpp).
  void _adoptVertexData(float *vertexData, long vertexCount,
                        const unsigned int *indexData, long indexCount,
                        int vertexStride);

  // Override OkObject's transform update
  void updateTransformSelf() override;

public:
  // Constructors.
  // vertexStride selects the input layout: 5 = x,y,z,u,v (normals are
  // computed here by accumulating face normals per vertex -- de-indexed
  // meshes get exact flat face normals, indexed meshes get smoothed
  // ones); 8 = x,y,z,u,v,nx,ny,nz (caller-provided normals, verbatim).
  // Internally vertices are always stored with stride 8.
  OkItem(const std::string &name, float *vertexData, long vertexCount,
         unsigned int *indexData, long indexCount, int vertexStride = 5);
  ~OkItem() override;

  // Delete copy constructor and assignment
  OkItem(const OkItem &)            = delete;
  OkItem &operator=(const OkItem &) = delete;

  // Geometry
  float getRadius() const {
    return radius;
  }
  void updateVertexData(float *newVertexData, long newVertexCount);

  // Texture methods
  void loadTextureFromFile(const std::string &texturePath);
  // Adopt a texture the caller already has a pointer to. The item takes
  // its own reference: the destructor releases one, so an item that only
  // borrowed the pointer would push the count below what the sharers
  // actually hold and free a texture still in use.
  void setTexture(const std::string &name, OkTexture *tex) {
    if (texture && !textureName.empty()) {
      OkTextureHandler::getInstance()->removeReference(textureName);
    }
    texture     = tex;
    textureName = name;
    if (texture && !textureName.empty()) {
      OkTextureHandler::getInstance()->addReference(textureName);
    }
  }

  // Flags
  // Add a material slot covering [firstIndex, firstIndex + indexCount)
  // of the index buffer, textured from `path`. Ranges are drawn in the
  // order added and should cover the buffer without overlapping; adding
  // none leaves the item single-material.
  void   addMaterialFromFile(long firstIndex, long indexCount,
                             const std::string &path);
  void   clearMaterials();
  size_t getMaterialCount() const {
    return materials.size();
  }

  void setWireframe(bool wireframe) {
    drawWireframe = wireframe;
  }
  bool getWireframe() const {
    return drawWireframe;
  }
  // Opt this item out of (or back into) the global wireframe switch.
  // Its own setWireframe still applies either way.
  void setWireframeGlobal(bool on) {
    wireframeGlobal = on;
  }
  // Take this item out of (or back into) the shadow maps. Light does
  // not cast shadows; matter does.
  void setCastsShadow(bool on) {
    castsShadow = on;
  }
  bool getCastsShadow() const {
    return castsShadow;
  }
  // Set by the shadow map around its own pass, so an item can tell
  // which pass is drawing it.
  static void setShadowPass(bool on);
  static bool inShadowPass();
  bool        getWireframeGlobal() const {
    return wireframeGlobal;
  }
  void setWireframeColor(float r, float g, float b) {
    wireframeColor[0] = r;
    wireframeColor[1] = g;
    wireframeColor[2] = b;
  }
  void setFillColor(float r, float g, float b) {
    setFillColor(r, g, b, 1.0f);
  }
  void setFillColor(float r, float g, float b, float a) {
    fillColor[0] = r;
    fillColor[1] = g;
    fillColor[2] = b;
    fillColor[3] = a;
  }
  // Tint multiplied over the texture in the fill pass (white = untouched).
  // Additive blending (glows/halos): drawn with src-alpha one blending
  // and no depth writes. World pass only.
  void setAdditive(bool on) {
    additive = on;
  }
  bool isBlended() const override {
    return additive;
  }
  // Unlit: this item ignores the Gouraud sun/point lights and the scene
  // tint (light sources must not be tinted by the atmosphere). World
  // pass only -- the flag restores world-pass uniforms after drawing.
  void setUnlit(bool on) {
    unlit = on;
  }

  // Masked materials: when the texture carries a material code in its
  // alpha channel (instead of opacity), each code takes its own tint,
  // so one texture serves many colour variants. Codes are read as
  // ~1.00, ~0.50 and ~0.25; anything below ~0.12 is discarded.
  void setMaskedMaterials(bool on) {
    maskedMaterials = on;
  }
  // Per-slot: false multiplies the tint over the texture (keeping its
  // hue), true keeps only the texture's luminance so the tint sets the
  // hue -- what an emissive surface wants, where the artwork gives the
  // shading and the tint gives the colour of the light.
  void setMaterialLuminance(int slot, bool on) {
    if (slot >= 0 && slot <= 2) {
      matLuma[slot] = on ? 1.0f : 0.0f;
    }
  }
  void setMaterialTint(int slot, float r, float g, float b) {
    if (slot < 0 || slot > 2) {
      return;
    }
    matTint[slot][0] = r;
    matTint[slot][1] = g;
    matTint[slot][2] = b;
  }

  void setTintColor(float r, float g, float b, float a) {
    tintColor[0] = r;
    tintColor[1] = g;
    tintColor[2] = b;
    tintColor[3] = a;
  }
  // Cross-fade for level-of-detail handovers: 1 draws the item whole,
  // 0 drops it entirely, and values between drop that share of its
  // pixels on an ordered pattern. Two versions of the same object can
  // therefore trade places gradually while both stay in the opaque
  // pass -- no blending, no sorting, depth buffer intact.
  void setFade(float f) {
    fade = std::min(std::max(f, 0.0f), 1.0f);
  }
  float getFade() const {
    return fade;
  }
  // The two sides of a handover must drop opposite pixels, or each
  // keeps the same half and the rest shows through to the background.
  // Set this on one of the pair, not on both.
  void setFadeInverted(bool on) {
    fadeInverted = on;
  }
  void setDrawMode(GLenum mode) {
    drawMode = mode;
  }
  GLenum getDrawMode() const {
    return drawMode;
  }
  void setVisible(bool visible) {
    this->visible = visible;
  }
  bool getVisible() const {
    return visible;
  }

  // Update and render
  void stepSelf(float dt) override;
  void drawSelf() override;
};

#endif
