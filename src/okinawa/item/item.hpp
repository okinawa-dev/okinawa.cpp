#ifndef OK_ITEM_HPP
#define OK_ITEM_HPP

#include "../core/gl_config.hpp"
#include "../core/object.hpp"
#include "../handlers/textures.hpp"
#include "../item/texture.hpp"
#include <string>

class OkItem : public OkObject {
private:
  void _initBuffers();

protected:
  // Mesh, material and GL state. Protected rather than private because
  // subclasses issue their own draws with the same state: OkBillboard
  // modulates the tint per frame, OkInstancedItem replaces the draw call
  // with an instanced one.
  bool   visible;
  bool   drawWireframe;  // Flag to control wireframe rendering
  GLenum drawMode;       // GL_TRIANGLES, GL_LINES, etc.

  // Flat fill colour when untextured, and wireframe line colour (RGB, white).
  float fillColor[4];       // RGBA: alpha honoured by blended passes (GUI)
  float wireframeColor[3];

  // Geometry
  float        *vertices;
  unsigned int *indices;
  long          numVertices;
  long          numIndices;
  float         radius;        // bounding-sphere radius (half bbox diagonal)
  float         sphereCenter[3];  // bounding-sphere centre, LOCAL coords
  bool          additive;      // additive blending (light halos)
  bool          unlit;         // skip Gouraud light AND scene tint
  int           nearLights[8];    // cached nearest point-light indices
  int           nearLightCount;
  long          nearLightGen;     // registry generation of the cache

  // OpenGL objects
  GLuint VAO, VBO, EBO;

  // Texture
  std::string textureName;  // Name/path of the texture for reference counting
  OkTexture  *texture;

  // Multiplies the texture in the fill pass.
  float tintColor[4];
  bool  maskedMaterials;
  float matTint[3][3];

  // Geometry
  void _calculateRadius();

  // Store vertex data with the internal stride-8 layout; stride-5 input
  // gets normals computed from the triangle list (see item.cpp).
  void _adoptVertexData(float *vertexData, long vertexCount,
                        unsigned int *indexData, long indexCount,
                        int vertexStride);

  // Override OkObject's transform update
  void updateTransformSelf() override;

public:
  // Constructors.
  // vertexStride selects the INPUT layout: 5 = x,y,z,u,v (normals are
  // computed here by accumulating face normals per vertex -- de-indexed
  // meshes get exact flat face normals, indexed meshes get smoothed
  // ones); 8 = x,y,z,u,v,nx,ny,nz (caller-provided normals, verbatim).
  // Internally vertices are ALWAYS stored with stride 8.
  OkItem(const std::string &name, float *vertexData, long vertexCount,
         unsigned int *indexData, long indexCount, int vertexStride = 5);
  ~OkItem();

  // Delete copy constructor and assignment
  OkItem(const OkItem &)            = delete;
  OkItem &operator=(const OkItem &) = delete;

  // Geometry
  float getRadius() const { return radius; }
  void  updateVertexData(float *newVertexData, long newVertexCount);

  // Texture methods
  void loadTextureFromFile(const std::string &texturePath);
  void setTexture(const std::string &name, OkTexture *tex) {
    if (texture && !textureName.empty()) {
      OkTextureHandler::getInstance()->removeReference(textureName);
    }
    texture     = tex;
    textureName = name;
  }

  // Flags
  void   setWireframe(bool wireframe) { drawWireframe = wireframe; }
  bool   getWireframe() const { return drawWireframe; }
  void   setWireframeColor(float r, float g, float b) {
    wireframeColor[0] = r;
    wireframeColor[1] = g;
    wireframeColor[2] = b;
  }
  void   setFillColor(float r, float g, float b) {
    setFillColor(r, g, b, 1.0f);
  }
  void   setFillColor(float r, float g, float b, float a) {
    fillColor[0] = r;
    fillColor[1] = g;
    fillColor[2] = b;
    fillColor[3] = a;
  }
  // Tint multiplied over the texture in the fill pass (white = untouched).
  // Additive blending (glows/halos): drawn with src-alpha ONE blending
  // and no depth writes. World pass only.
  void setAdditive(bool on) { additive = on; }
  bool isBlended() const override { return additive; }
  // Unlit: this item ignores the Gouraud sun/point lights AND the scene
  // tint (light sources must not be tinted by the atmosphere). World
  // pass only -- the flag restores world-pass uniforms after drawing.
  void setUnlit(bool on) { unlit = on; }

  // Masked materials: when the texture carries a MATERIAL CODE in its
  // alpha channel (instead of opacity), each code takes its own tint,
  // so one texture serves many colour variants. Codes are read as
  // ~1.00, ~0.50 and ~0.25; anything below ~0.12 is discarded.
  void setMaskedMaterials(bool on) { maskedMaterials = on; }
  void setMaterialTint(int slot, float r, float g, float b) {
    if (slot < 0 || slot > 2) {
      return;
    }
    matTint[slot][0] = r;
    matTint[slot][1] = g;
    matTint[slot][2] = b;
  }

  void   setTintColor(float r, float g, float b, float a) {
    tintColor[0] = r;
    tintColor[1] = g;
    tintColor[2] = b;
    tintColor[3] = a;
  }
  void   setDrawMode(GLenum mode) { drawMode = mode; }
  GLenum getDrawMode() const { return drawMode; }
  void   setVisible(bool visible) { this->visible = visible; }
  bool   getVisible() const { return visible; }

  // Update and render
  void stepSelf(float dt) override;
  void drawSelf() override;
};

#endif
