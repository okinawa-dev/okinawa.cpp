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

  // Flags
  bool   visible;
  bool   drawWireframe;  // Flag to control wireframe rendering
  GLenum drawMode;       // GL_TRIANGLES, GL_LINES, etc.

  // Flat fill colour when untextured, and wireframe line colour (RGB, white).
  float fillColor[4];       // RGBA: alpha honoured by blended passes (GUI)
  float wireframeColor[3];
  float tintColor[4];       // multiplies the texture in the fill pass

  // Geometry
  float        *vertices;
  unsigned int *indices;
  long          numVertices;
  long          numIndices;
  float         radius;  // Maximum dimension

  // OpenGL objects
  GLuint VAO, VBO, EBO;

  // Texture
  std::string textureName;  // Name/path of the texture for reference counting
  OkTexture  *texture;

protected:
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
