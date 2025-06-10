#ifndef OK_OBJECT_HPP
#define OK_OBJECT_HPP

#include "../math/point.hpp"
#include "../math/rotation.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

class OkObject {
protected:
  std::string name;

  OkPoint    position;
  OkRotation rotation;
  OkPoint    scaling;

  // Physics
  OkPoint speed;
  float   maxVel;
  float   accel;

  // Rotation velocities
  OkPoint vRot;
  OkPoint maxVRot;
  OkPoint accelRot;

  // Hierarchy
  OkObject *_parent;
  OkObject *_firstChild;
  OkObject *_nextSibling;

  // Flags
  bool drawOriginAxis;  // Flag to draw origin axis

  // Pure virtual method for derived classes to implement their specific drawing
  // and update
  virtual void drawSelf()            = 0;
  virtual void stepSelf(float dt)    = 0;
  virtual void updateTransformSelf() = 0;

public:
  OkObject(const std::string &name);
  virtual ~OkObject();

  // Position
  OkPoint getPosition() const;
  void    setPosition(float x, float y, float z);
  void    setPosition(const OkPoint &newPosition);
  void    move(float dx, float dy, float dz);

  void setDrawOriginAxis(bool drawAxis) { drawOriginAxis = drawAxis; }
  bool getDrawOriginAxis() const { return drawOriginAxis; }
  void drawAxis() const;

  // Rotation
  OkRotation getRotation() const;
  void       setRotation(float x, float y, float z);
  void       setRotation(const OkRotation &newRotation);
  void       rotate(float dx, float dy, float dz);

  // Scale
  OkPoint getScaling() const { return scaling; }
  void    setScaling(float x, float y, float z) { scaling = OkPoint(x, y, z); }

  // Physics
  OkPoint getSpeed() const { return speed; }
  void    setSpeed(float x, float y, float z) { speed = OkPoint(x, y, z); }
  float   getSpeedMagnitude() const { return speed.magnitude(); }

  void setMaxVelocity(float maxVelocity) { maxVel = maxVelocity; }
  void setAcceleration(float acceleration) { accel = acceleration; }

  // Getters
  const std::string &getName() const { return name; }

  // Hierarchy
  void      attach(OkObject *object);
  void      attachTo(OkObject *parent);
  void      detachFromParent();
  void      detachAllChildren();
  OkObject *getNextSibling() const { return _nextSibling; }
  OkObject *getFirstChild() const { return _firstChild; }
  OkObject *getParent() const { return _parent; }

  // Transform matrix
  glm::mat4 getTransformMatrix() const;

  // Final transform update that enforces hierarchy
  virtual void updateTransform() final;

  // Final step method that enforces the update sequence
  virtual void step(float dt) final;

  // Final draw method that enforces the drawing sequence
  virtual void draw() final;
};

#endif
