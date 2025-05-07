#ifndef OK_ITEM_HPP
#define OK_ITEM_HPP

#if (__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
// TODO this for other platforms
#include <GL/glew.h>
#endif

#include "../item/texture.hpp"
#include "../math/point.hpp"
#include "../math/rotation.hpp"
// #include "../texture/texture.hpp"
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

class OkItem {
private:
  void _initBuffers();

  std::string name;

  // Flags
  bool visible;
  bool drawWireframe;  // Flag to control wireframe rendering

  // Geometry
  float        *vertices;
  unsigned int *indices;
  long          numVertices;
  long          numIndices;
  float         radius;  // Maximum dimension

  // OpenGL objects
  GLuint     VAO, VBO, EBO;
  OkTexture *texture;

  // Transform
  OkPoint position;
  OkPoint size;
  OkPoint scaling;

  // Physics
  OkPoint speed;
  float   maxVel;
  float   accel;

  // Rotation
  OkPoint    vRot;
  OkPoint    maxVRot;
  OkPoint    accelRot;
  OkRotation rotation;

protected:
  // Geometry
  void _calculateRadius();

  // Hierarchy
  OkItem *_attachedItems;  // linked list
  OkItem *_nextItem;       // next sibling
  OkItem *_parentItem;     // parent item
  OkItem *_getFirstAttached() const;

  void _createModelMatrix(glm::mat4 &model) const;

public:
  // Constructors
  OkItem(const std::string &name, float *vertices, long vertexCount,
         unsigned int *indices, long indexCount);
  ~OkItem();

  // Delete copy constructor and assignment
  OkItem(const OkItem &)            = delete;
  OkItem &operator=(const OkItem &) = delete;

  // Geometry
  float getRadius() const {
    return radius;
  }

  // Texture methods
  void setTexture(const std::string &texturePath);

  // Getters and setters
  OkPoint getPosition() const;
  void    setPosition(float x, float y, float z);

  OkPoint getScaling() const;
  void    setScaling(float x, float y, float z);

  OkPoint getSpeed() const;
  void    setSpeed(float x, float y, float z);
  float   getSpeedMagnitude() const;

  // Movement and rotation
  void       move(float dx, float dy, float dz);
  void       rotate(float dx, float dy, float dz);
  void       setRotation(float x, float y, float z);
  OkRotation getRotation() const;

  void setWireframe(bool wireframe) {
    drawWireframe = wireframe;
  }

  // Hierarchy methods
  void attach(OkItem *child);
  void detach(OkItem *child);
  void detachAll();

  OkItem *getParent();

  // Update and render
  void step(float dt);
  void draw();
};

#endif
