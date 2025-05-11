#ifndef OK_OBJECT_HPP
#define OK_OBJECT_HPP

#include "../config/config.hpp"
#include "../math/point.hpp"
#include "../math/rotation.hpp"
#include "../utils/logger.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
  virtual void updateTransform() final {
    // First update our local transform
    updateTransformSelf();

    // Then recursively update all children's transforms
    OkObject *current = _firstChild;
    while (current != nullptr) {
      current->updateTransform();
      current = current->getNextSibling();
    }
  }

  // Final step method that enforces the update sequence
  virtual void step(float dt) final {
    float frameTime = dt / OkConfig::getFloat("graphics.time-per-frame");

    // Process movement if there's any speed
    if (speed.x() != 0 || speed.y() != 0 || speed.z() != 0) {
      // Check if speed exceeds maxVel
      if (maxVel > 0.0f) {
        float currentSpeed = speed.magnitude();
        if (currentSpeed > maxVel) {
          speed = speed.normalize() * maxVel;
        }
      }
      move(speed.x() * frameTime, speed.y() * frameTime, speed.z() * frameTime);
    }

    // Process rotation if there's any rotational speed
    if (vRot.x() != 0 || vRot.y() != 0 || vRot.z() != 0) {
      rotate(vRot.x() * frameTime, vRot.y() * frameTime, vRot.z() * frameTime);
    }

    // Call the derived class's specific update logic
    stepSelf(dt);

    // Update children recursively (this stays in OkObject)
    OkObject *current = _firstChild;
    while (current != nullptr) {
      current->step(dt);
      current = current->getNextSibling();
    }
  }

  // Final draw method that enforces the drawing sequence
  virtual void draw() final {
    // OkLogger::info("Drawing object in hierarchy, name: " + name);

    // Call the derived class's specific drawing logic
    drawSelf();

    // Draw children recursively (this stays in OkObject)
    OkObject *current = _firstChild;
    while (current != nullptr) {
      current->draw();
      current = current->getNextSibling();
    }
  }
};

#endif
