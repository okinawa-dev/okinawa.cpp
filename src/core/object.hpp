#ifndef OK_OBJECT_HPP
#define OK_OBJECT_HPP

#include "../math/point.hpp"
#include "../math/rotation.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class OkObject {
protected:
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

public:
  OkObject();
  virtual ~OkObject();

  // Position
  OkPoint getPosition() const;
  void    setPosition(float x, float y, float z);
  void    setPosition(const OkPoint &newPosition);
  void    move(float dx, float dy, float dz);

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

  // Hierarchy
  void      attachTo(OkObject *parent);
  void      detachFromParent();
  void      detachAllChildren();
  OkObject *getNextSibling() const { return _nextSibling; }
  OkObject *getFirstChild() const { return _firstChild; }
  OkObject *getParent() const { return _parent; }

  // Transform matrix
  glm::mat4 getTransformMatrix() const;

  virtual void updateTransform() = 0;
};

#endif
