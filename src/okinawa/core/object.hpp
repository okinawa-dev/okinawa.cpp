#ifndef OK_OBJECT_HPP
#define OK_OBJECT_HPP

#include "../math/point.hpp"
#include "../math/rotation.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

class OkObject {
public:
  // WHICH DEBUG DRAWING. One entry per thing an object can show about
  // itself, and the list is meant to grow: a bounding box, a direction
  // vector, whatever the next question about "where is this actually"
  // turns out to be.
  enum DebugHelper {
    DEBUG_ORIGIN = 0,  // the three axes at its origin
    DEBUG_SPHERE,      // the sphere it claims to fit in
    DEBUG_BOX,         // the box around that sphere
    DEBUG_CENTRE,      // where that sphere is centred
    DEBUG_HELPER_COUNT
  };

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

  // The pointer surgery of leaving a parent, without the transform
  // update that follows it. The destructor needs exactly this: updating
  // a transform is a virtual call, and by then there is no object left
  // to answer it.
  void unlinkFromParent();

  // Flags
  bool drawOriginAxis;  // this one always shows its origin, switch or not

  // The world's switches, read from the config once a frame rather than
  // per object: a lookup by string, fourteen thousand times a frame, is
  // milliseconds spent on a switch that is usually off.
  static bool _debugHelpers[DEBUG_HELPER_COUNT];

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

  // THE DEBUG DRAWINGS AN OBJECT MAKES OF ITSELF.
  //
  // Its origin, the sphere it claims to fit in, the box around that,
  // the point the sphere is centred on -- and whatever is added next.
  // They answer "where is this actually", and that question only has an
  // answer when a whole world answers it at once: one object's axes say
  // nothing about whether it is anchored the way its neighbours are.
  //
  // WHICH IS WHY EACH OBJECT ASKS, AND NOTHING PUSHES IT IN. Walking
  // the scene to switch a flag on in every object is the scene knowing
  // about gizmos, which is not its business, and it is state to keep in
  // step with -- the next object attached after the walk is not in
  // step. Here the switches are read once a frame and every object
  // consults them while it draws itself.
  /** @brief Read the `debug.*` switches. Called once a frame. */
  static void refreshDebugHelpers();
  static bool debugHelperOn(DebugHelper which) {
    return _debugHelpers[which];
  }

  // A LINE ADDED, NOT A LINE DRAWN.
  //
  // An object that issues its own draw call for a gizmo costs a draw
  // call and -- as `drawAxis` did -- a VAO and a VBO created and
  // destroyed for three lines, every object, every frame. Fourteen
  // thousand of those is not a debugging view, it is a stall. So the
  // objects fill one buffer and the frame draws it once.
  static void addDebugLine(const OkPoint &from, const OkPoint &to, float r,
                           float g, float b);
  /** @brief Draw everything the objects added, and empty the buffer. */
  static void flushDebugHelpers();

  /**
   * @brief What this object adds about itself. Extended by subclasses.
   *
   * The base knows where it is and how it is turned, so it draws the
   * axes at its origin. What an object IS -- how far it reaches, where
   * its geometry sits inside it -- is known further down, and that is
   * where the sphere and the box come from.
   */
  virtual void drawDebugHelpers() const;

  /**
   * @brief How far this object reaches from its own centre, in metres.
   *
   * Zero for an object with no geometry of its own -- a group, a node
   * that is only somewhere to be. What has a mesh answers with its
   * sphere, and the debug drawings are sized from this so that a lamp
   * post and a district are both legible.
   */
  virtual float getRadius() const {
    return 0.0f;
  }

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
