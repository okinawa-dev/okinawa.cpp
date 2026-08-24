#ifndef OK_ITEM_HPP
#define OK_ITEM_HPP

#include "../core/gl_config.hpp"
#include "../core/object.hpp"
#include "../handlers/textures.hpp"
#include "../item/texture.hpp"
#include "../math/ray.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <vector>

class OkItem : public OkObject {
public:
  // A vertex as a caller hands it over, and as the item stores it.
  //
  // Input carries a position and a texture coordinate; the item keeps
  // those plus a normal, which it works out from the triangle list when
  // the caller does not supply one. Every index into the vertex array is
  // a multiple of one of these, which is why they have names instead of
  // being written as 5 and 8 in forty places.
  static const int VERTEX_STRIDE_IN = 5;
  static const int VERTEX_STRIDE    = 8;
  // Where the normal begins inside a stored vertex.
  static const int VERTEX_NORMAL = 5;
  // A colour, with and without its alpha, and how many material slots an
  // item can tint independently.
  static const int RGB       = 3;
  static const int RGBA      = 4;
  static const int MAT_SLOTS = 3;

private:
  void        _initBuffers();
  void        _initDefaults();
  static void _expandVertices(const float *vertexData, long vertexCount,
                              const unsigned int *indexData, long indexCount,
                              int vertexStride, std::vector<float> *out);

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
  std::array<float, RGBA>
      fillColor;  // RGBA: alpha honoured by blended passes (GUI)
  std::array<float, RGB> wireframeColor;

  // Geometry
  float                 *vertices;
  unsigned int          *indices;
  long                   numVertices;
  long                   numIndices;
  float                  radius;  // bounding-sphere radius (half bbox diagonal)
  std::array<float, RGB> sphereCenter;  // bounding-sphere centre, local coords
  bool                   additive;      // additive blending (light halos)
  bool                   unlit;         // skip Gouraud light and the scene tint
  // How many point lights an item keeps track of. Beyond this the
  // nearest ones win and the rest are ignored for that item.
  static const int                 MAX_NEAR_LIGHTS = 8;
  std::array<int, MAX_NEAR_LIGHTS> nearLights;  // nearest point lights
  int                              nearLightCount;
  long nearLightGen;  // registry generation of the cache

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
  std::array<float, RGBA> tintColor;
  bool                    maskedMaterials;
  float                   fade;          // 1 = solid; below that, dithered away
  bool                    fadeInverted;  // use the opposite half of the pattern
  std::array<std::array<float, RGB>, MAT_SLOTS> matTint;
  std::array<float, MAT_SLOTS>                  matLuma;

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
         unsigned int *indexData, long indexCount,
         int vertexStride = VERTEX_STRIDE_IN);

  /**
   * @brief An item with no geometry yet, filled by addMesh().
   *
   * For a mesh made of pieces that wear different textures. Nothing is
   * drawn until upload() has been called.
   */
  explicit OkItem(const std::string &name);

  /**
   * @brief Appends a piece of mesh wearing one texture.
   *
   * The piece keeps its own texture as a material range, so a single
   * item can carry as many as it is given: one transform, one bounding
   * sphere, one culling test, one draw per texture. Built as an item
   * per texture instead, a mesh of eight materials is eight objects.
   *
   * The indices are the piece's own, counted from its first vertex; they
   * are moved along by what is already in the item. That arithmetic is
   * the reason this exists -- every caller that merged meshes by hand
   * wrote it out again, and an index that is wrong but still in range
   * draws the wrong geometry without failing.
   *
   * @param vertexData   The piece's vertices, `vertexStride` floats each.
   * @param vertexCount  How many floats, not how many vertices.
   * @param indexData    Its indices, from zero.
   * @param indexCount   How many of them.
   * @param texturePath  The texture this piece wears.
   * @param vertexStride 5 for x,y,z,u,v (normals computed here), 8 for
   *                     caller-provided normals.
   */
  void addMesh(const float *vertexData, long vertexCount,
               const unsigned int *indexData, long indexCount,
               const std::string &texturePath,
               int                vertexStride = VERTEX_STRIDE_IN);

  /** @brief Hands the assembled mesh to the GPU. Call once, at the end. */
  void upload();
  ~OkItem() override;

  // Delete copy constructor and assignment
  OkItem(const OkItem &)            = delete;
  OkItem &operator=(const OkItem &) = delete;

  // Geometry
  float getRadius() const {
    return radius;
  }
  // How many indices the mesh has, which is three per triangle. Reported
  // rather than kept to itself because a count is not the geometry: it
  // is what a caption, a budget or a test says about it, and working it
  // out again from the file the mesh came from is a second answer to a
  // question the item already knows.
  long getIndexCount() const {
    return numIndices;
  }
  void updateVertexData(float *newVertexData, long newVertexCount);

  /**
   * @brief Where a ray crosses this item's own triangles, or false if it
   *        misses.
   *
   *        The item is the only one that can answer this: it owns its
   *        vertex and index buffers and its place in the world, and hands
   *        neither back. Anything outside would have to keep a second
   *        copy of the mesh to ask the question, which is tens of
   *        megabytes for a city and a copy that can fall out of step with
   *        what is drawn.
   *
   *        Two tests, in the order that makes the second one cheap: the
   *        bounding sphere the frustum already culls with, and only then
   *        the triangles. Both run in the item's local space, reached by
   *        transforming the ray rather than the mesh -- one matrix
   *        against one ray instead of one matrix against every vertex.
   *
   *        It answers about geometry and nothing else. Whether the item
   *        is visible, whether it is the sort of thing this application
   *        lets a user choose, and which of several hits wins are all
   *        decisions for the caller; an engine that made them here would
   *        be making them for every application at once.
   *
   *        Items drawn as lines or points are never hit: there is no
   *        surface to cross. A subclass that draws its mesh more than
   *        once (instanced items) is answered for the base copy only.
   *
   * @param ray         In world space. A unit direction makes the
   *                    distance a world distance; see OkRay.
   * @param outDistance If non-null, the distance to the nearest crossing.
   */
  bool intersectRay(const OkRay &ray, float *outDistance) const;

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
