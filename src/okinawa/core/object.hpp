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

  void setDrawOriginAxis(bool drawAxis) {
    drawOriginAxis = drawAxis;
  }
  bool getDrawOriginAxis() const {
    return drawOriginAxis;
  }
  void drawAxis() const;

  // Rotation
  OkRotation getRotation() const;
  void       setRotation(float x, float y, float z);
  void       setRotation(const OkRotation &newRotation);
  void       rotate(float dx, float dy, float dz);

  // Scale
  OkPoint getScaling() const {
    return scaling;
  }
  void setScaling(float x, float y, float z) {
    scaling = OkPoint(x, y, z);
  }

  // Physics
  OkPoint getSpeed() const {
    return speed;
  }
  void setSpeed(float x, float y, float z) {
    speed = OkPoint(x, y, z);
  }
  float getSpeedMagnitude() const {
    return speed.magnitude();
  }

  void setMaxVelocity(float maxVelocity) {
    maxVel = maxVelocity;
  }
  void setAcceleration(float acceleration) {
    accel = acceleration;
  }

  // Getters
  const std::string &getName() const {
    return name;
  }

  // Hierarchy
  void      attach(OkObject *object);
  void      attachTo(OkObject *parent);
  void      detachFromParent();
  void      detachAllChildren();
  OkObject *getNextSibling() const {
    return _nextSibling;
  }
  OkObject *getFirstChild() const {
    return _firstChild;
  }
  OkObject *getParent() const {
    return _parent;
  }

  // Transform matrix
  glm::mat4 getTransformMatrix() const;

  // Final transform update that enforces hierarchy
  virtual void updateTransform() final;

  // Final step method that enforces the update sequence
  virtual void step(float dt) final;

  // Final draw method that enforces the drawing sequence
  virtual void draw() final;

  /**
   * @brief Draw this subtree, but only what belongs to one pass.
   *
   * Opaque geometry is drawn nearest first, so the depth buffer can
   * reject what is hidden; blended geometry -- halos, glows -- goes
   * afterwards, because it deliberately does not write depth and any
   * opaque surface drawn later would paint over it.
   *
   * Which pass an object belongs to is its own business (`isBlended`),
   * and it is asked per OBJECT rather than per root. It used to be
   * decided for a whole root: harmless while every object was a root,
   * and wrong the moment one of them has children -- a single halo
   * inside a group would carry all of that group's opaque geometry into
   * the late pass.
   *
   * @param blendedPass true to draw the blended half, false the opaque
   *        half. The traversal walks the whole subtree either way; each
   *        object draws itself only in its own pass.
   */
  virtual void drawPass(bool blendedPass) final;

  /**
   * @brief Whether this object and its children are drawn at all.
   *
   * Asked before anything else, every frame. The default is yes; a node
   * that stands for a region of the world answers no when that region
   * is behind the viewer or beyond the distance it is drawn at, and its
   * whole subtree costs one test instead of one per item.
   *
   * A question rather than a flag on purpose: a flag has to be put back,
   * and a flag left false is a piece of the world that disappears with
   * nothing to say why.
   */
  virtual bool shouldDraw() const {
    return true;
  }

  /**
   * @brief Whether this object and its children are stepped at all.
   *
   * The same question for the update: what is not near enough to be
   * looked at is rarely near enough to need moving.
   */
  virtual bool shouldStep(float dt) const {
    (void)dt;
    return true;
  }

  // Blended/additive objects (light halos, glows) must be drawn AFTER
  // all opaque geometry: they deliberately do not write depth, so any
  // opaque surface drawn later would pass the depth test and paint over
  // them. OkScene::draw uses this to order the two passes; subtrees
  // report true when any descendant is blended.
  virtual bool isBlended() const {
    return false;
  }
};

#endif
