#ifndef OK_ITEM_HPP
#define OK_ITEM_HPP

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

  // Override OkObject's transform update
  void updateTransformSelf() override;

public:
  // Constructors
  OkItem(const std::string &name, float *vertexData, long vertexCount,
         unsigned int *indexData, long indexCount);
  ~OkItem();

  // Delete copy constructor and assignment
  OkItem(const OkItem &)            = delete;
  OkItem &operator=(const OkItem &) = delete;

  // Geometry
  float getRadius() const { return radius; }

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
  void   setDrawMode(GLenum mode) { drawMode = mode; }
  GLenum getDrawMode() const { return drawMode; }
  void   setVisible(bool visible) { this->visible = visible; }
  bool   getVisible() const { return visible; }

  // Update and render
  void stepSelf(float dt) override;
  void drawSelf() override;
};

#endif
