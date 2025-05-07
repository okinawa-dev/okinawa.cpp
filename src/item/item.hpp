#ifndef OK_ITEM_HPP
#define OK_ITEM_HPP

#if (__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#include "../core/object.hpp"
#include "../item/texture.hpp"
#include "../math/point.hpp"
#include "../math/rotation.hpp"
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

class OkItem : public OkObject {
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

  // Physics
  OkPoint speed;
  float   maxVel;
  float   accel;

  // Rotation velocities
  OkPoint vRot;
  OkPoint maxVRot;
  OkPoint accelRot;

protected:
  // Geometry
  void _calculateRadius();

  // Override OkObject's transform update
  void updateTransform() override;

public:
  // Constructors
  OkItem(const std::string &name, float *vertices, long vertexCount,
         unsigned int *indices, long indexCount);
  ~OkItem();

  // Delete copy constructor and assignment
  OkItem(const OkItem &)            = delete;
  OkItem &operator=(const OkItem &) = delete;

  // Geometry
  float getRadius() const { return radius; }

  // Texture methods
  void setTexture(const std::string &texturePath);

  // Physics
  OkPoint getSpeed() const;
  void    setSpeed(float x, float y, float z);
  float   getSpeedMagnitude() const;

  void setWireframe(bool wireframe) { drawWireframe = wireframe; }

  // Update and render
  void step(float dt);
  void draw();
};

#endif
