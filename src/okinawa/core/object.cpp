#include "object.hpp"
#include "../config/config.hpp"
#include "gl_config.hpp"
#include "math/point.hpp"
#include "math/rotation.hpp"
#include <array>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>

/**
 * @brief Constructor for the OkObject class.
 */
OkObject::OkObject(const std::string &name) {

  // Direct assignment - string will handle copying internally
  this->name = name;

  position = OkPoint(0.0f, 0.0f, 0.0f);
  scaling  = OkPoint(1.0f, 1.0f, 1.0f);
  speed    = OkPoint(0.0f, 0.0f, 0.0f);
  maxVel   = 0.0f;
  accel    = 0.0f;
  vRot     = OkPoint(0.0f, 0.0f, 0.0f);
  maxVRot  = OkPoint(0.0f, 0.0f, 0.0f);
  accelRot = OkPoint(0.0f, 0.0f, 0.0f);

  _parent      = nullptr;
  _firstChild  = nullptr;
  _nextSibling = nullptr;

  drawOriginAxis = false;  // Default to not showing axes
}

/**
 * @brief Destructor for the OkObject class.
 *        Cleans up the object and detaches from parent.
 */
OkObject::~OkObject() {
  // Unlinked, not detached: `detachFromParent` ends by recomputing this
  // object's transform, which is a virtual call, and a virtual call from
  // a destructor lands on a method whose object no longer exists. The
  // program dies with "pure virtual function called" and no clue as to
  // which one.
  //
  // It never happened while nothing in the world had a parent. The first
  // thing to have one -- a cell holding what stands in it -- brought the
  // whole city down the moment a cell was unloaded.
  unlinkFromParent();
  detachAllChildren();
}

/**
 * @brief Get the position of the object in world coordinates.
 *        If the object has a parent, the position is transformed by the
 *        parent's rotation and position.
 * @return The world position of the object.
 */
OkPoint OkObject::getPosition() const {
  if (_parent) {
    // Transform local position by parent's transform
    OkPoint worldPos = _parent->getRotation().transformPoint(position);
    return worldPos + _parent->getPosition();
  }
  return position;
}

/**
 * @brief Set the position of the object in local coordinates.
 *        This method updates the position and recalculates the transform
 *        matrix.
 * @param x The x-coordinate of the new position.
 * @param y The y-coordinate of the new position.
 * @param z The z-coordinate of the new position.
 */
void OkObject::setPosition(float x, float y, float z) {
  position = OkPoint(x, y, z);
  updateTransform();
}

/**
 * @brief Set the position of the object using an OkPoint.
 *        This method updates the position and recalculates the transform
 *        matrix.
 * @param newPosition The new position as an OkPoint.
 */
void OkObject::setPosition(const OkPoint &newPosition) {
  // OkPoint copy assignment operator
  position = newPosition;
  updateTransform();
}

/**
 * @brief Move the object by a specified distance in local coordinates.
 *       This method updates the position and recalculates the transform
 *       matrix.
 * @param dx The distance to move in the x-direction.
 * @param dy The distance to move in the y-direction.
 * @param dz The distance to move in the z-direction.
 */
void OkObject::move(float dx, float dy, float dz) {
  position = position + OkPoint(dx, dy, dz);
  updateTransform();
}

/**
 * @brief Get the rotation of the object in world coordinates.
 *        If the object has a parent, the rotation is transformed by the
 *        parent's rotation.
 * @return The world rotation of the object.
 */
OkRotation OkObject::getRotation() const {
  if (_parent) {
    return _parent->getRotation().combine(rotation);
  }
  return rotation;
}

/**
 * @brief Set the rotation of the object in local coordinates.
 *        This method updates the rotation and recalculates the transform
 *        matrix.
 * @param x The rotation around the x-axis in degrees.
 * @param y The rotation around the y-axis in degrees.
 * @param z The rotation around the z-axis in degrees.
 */
void OkObject::setRotation(float x, float y, float z) {
  rotation.setRotation(x, y, z);
  updateTransform();
}

/**
 * @brief Set the rotation of the object using an OkRotation.
 *        This method updates the rotation and recalculates the transform
 *        matrix.
 * @param newRotation The new rotation as an OkRotation.
 */
void OkObject::setRotation(const OkRotation &newRotation) {
  // OkPoint copy assignment operator
  rotation = newRotation;
  updateTransform();
}

/**
 * @brief Rotate the object by a specified angle in local coordinates.
 *        This method updates the rotation and recalculates the transform
 *        matrix.
 * @param dx The rotation around the x-axis in degrees.
 * @param dy The rotation around the y-axis in degrees.
 * @param dz The rotation around the z-axis in degrees.
 */
void OkObject::rotate(float dx, float dy, float dz) {
  rotation.rotate(dx, dy, dz);
  updateTransform();
}

/**
 * @brief Attach an object to this object.
 *        This method updates the parent-child relationship and recalculates
 *        the transform matrix.
 * @param object The object to attach.
 */
void OkObject::attach(OkObject *object) {
  if (object == nullptr)
    return;

  // Attach the object to this object
  object->attachTo(this);
}

/**
 * @brief Attach this object to a parent object.
 *        This method updates the parent-child relationship and recalculates
 *        the transform matrix.
 */
void OkObject::attachTo(OkObject *parent) {
  if (_parent == parent)
    return;

  detachFromParent();

  if (parent) {
    _parent             = parent;
    _nextSibling        = parent->_firstChild;
    parent->_firstChild = this;
  }

  updateTransform();
}

/**
 * @brief Detach this object from its parent.
 *        This method updates the parent-child relationship and recalculates
 *        the transform matrix.
 */
void OkObject::unlinkFromParent() {
  if (!_parent)
    return;

  // Find and remove this from parent's children
  OkObject **curr = &_parent->_firstChild;
  while (*curr && *curr != this) {
    curr = &(*curr)->_nextSibling;
  }

  if (*curr) {
    *curr = _nextSibling;
  }

  _parent      = nullptr;
  _nextSibling = nullptr;
}

void OkObject::detachFromParent() {
  if (!_parent)
    return;
  unlinkFromParent();
  // Its transform was measured from a parent it no longer has.
  updateTransform();
}

/**
 * @brief Detach all children from this object.
 *        This method updates the parent-child relationship for all children.
 */
void OkObject::detachAllChildren() {
  while (_firstChild) {
    _firstChild->detachFromParent();
  }
}

/**
 * @brief Get the transformation matrix for this object.
 *        This method combines the parent's transformation with the local
 *        transformation.
 * @return The transformation matrix as a glm::mat4.
 */
glm::mat4 OkObject::getTransformMatrix() const {
  // First build local transform in correct order
  glm::mat4 localMatrix(1.0f);

  // 1. First translate (move to position)
  localMatrix = glm::translate(
      localMatrix, glm::vec3(position.x(), position.y(), position.z()));

  // 2. Then rotate (around position)
  localMatrix = localMatrix * rotation.getMatrix();

  // 3. Finally scale (from position)
  localMatrix =
      glm::scale(localMatrix, glm::vec3(scaling.x(), scaling.y(), scaling.z()));

  // Apply parent transform if exists (parent * local for proper inheritance)
  if (_parent) {
    return _parent->getTransformMatrix() * localMatrix;
  }

  return localMatrix;
}

// Final transform update that enforces hierarchy
void OkObject::updateTransform() {
  // First update our local transform
  updateTransformSelf();

  // Then recursively update all children's transforms
  OkObject *current = _firstChild;
  while (current != nullptr) {
    current->updateTransform();
    current = current->getNextSibling();
  }
}

/**
 * @brief Update the object's state for the current frame.
 *        This method processes movement and rotation based on speed and
 *        rotational speed, and calls the derived class's specific update logic.
 *        It also updates all children recursively.
 * @param dt The time elapsed since the last frame.
 */
void OkObject::step(float dt) {
  if (!shouldStep(dt)) {
    return;
  }

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

/**
 * @brief Draw the object and its children recursively.
 *        This method first calls the derived class's specific drawing logic.
 */
void OkObject::draw() {
  // Both passes in one go, in the right order, for a caller that draws a
  // subtree on its own -- an inspection view, a thumbnail. A frame does
  // not come through here: OkScene draws the world one pass at a time,
  // so that ALL the opaque geometry is down before any of the blended.
  drawPass(false);
  drawPass(true);
}

void OkObject::drawPass(bool blendedPass) {
  // The question comes first, and it covers the children too: a region
  // of the world that is not drawn costs one test, not one per item.
  if (!shouldDraw()) {
    return;
  }

  // Each object draws itself in its own pass and stays out of the other.
  if (isBlended() == blendedPass) {
    drawSelf();
    drawDebugHelpers();
  }

  // Children recursively (this stays in OkObject)
  OkObject *current = _firstChild;
  while (current != nullptr) {
    current->drawPass(blendedPass);
    current = current->getNextSibling();
  }
}

namespace {

  // The lines every object added this frame, one bucket per colour.
  // Bucketed rather than carrying a colour per vertex because the world
  // shader takes its wireframe colour as a uniform: three or four
  // buckets is three or four draw calls, and a colour per vertex would
  // be a shader of its own for a debugging view.
  struct DebugBucket {
    float              r, g, b;
    std::vector<float> verts;  // x, y, z per point, two points per line
  };

  std::vector<DebugBucket> &debugBuckets() {
    static std::vector<DebugBucket> held;
    return held;
  }

  // How long the axes are drawn, in metres, and the bounds they are
  // held between. Scaled to the object rather than fixed: they used to
  // be a hundred units long whatever they belonged to, which over a
  // city of windows is a hairball rather than a diagram.
  const float AXIS_FRACTION = 0.5f;
  const float AXIS_MIN_M    = 0.25f;
  const float AXIS_MAX_M    = 8.0f;

  // How many segments a debug circle is drawn with. Twelve reads as a
  // circle at any distance worth looking at one from.
  const int CIRCLE_STEPS = 12;

}  // namespace

bool OkObject::_debugHelpers[OkObject::DEBUG_HELPER_COUNT] = {false, false,
                                                              false, false};

void OkObject::refreshDebugHelpers() {
  _debugHelpers[DEBUG_ORIGIN] = OkConfig::getBool("debug.origins");
  _debugHelpers[DEBUG_SPHERE] = OkConfig::getBool("debug.spheres");
  _debugHelpers[DEBUG_BOX]    = OkConfig::getBool("debug.boxes");
  _debugHelpers[DEBUG_CENTRE] = OkConfig::getBool("debug.centres");
}

void OkObject::addDebugLine(const OkPoint &from, const OkPoint &to, float r,
                            float g, float b) {
  std::vector<DebugBucket> &buckets = debugBuckets();
  DebugBucket              *into    = nullptr;
  for (size_t i = 0; i < buckets.size(); i++) {
    if (buckets[i].r == r && buckets[i].g == g && buckets[i].b == b) {
      into = &buckets[i];
      break;
    }
  }
  if (into == nullptr) {
    DebugBucket made;
    made.r = r;
    made.g = g;
    made.b = b;
    buckets.push_back(made);
    into = &buckets.back();
  }
  into->verts.push_back(from.x());
  into->verts.push_back(from.y());
  into->verts.push_back(from.z());
  into->verts.push_back(to.x());
  into->verts.push_back(to.y());
  into->verts.push_back(to.z());
}

/**
 * @brief What this object shows about itself: its origin, for the base.
 *
 * In WORLD coordinates, because everything added this frame is drawn in
 * one call with one model matrix -- the identity. An object that drew
 * its own gizmo under its own matrix cost a draw call and a pair of GL
 * buffers created and destroyed for three lines, which is what this
 * replaced.
 */
void OkObject::drawDebugHelpers() const {
  if (!drawOriginAxis && !_debugHelpers[DEBUG_ORIGIN]) {
    return;
  }
  glm::mat4 model = getTransformMatrix();
  glm::vec4 at    = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  // As long as the object is big, within reason: a lamp post and a
  // district cannot share one length and both be read.
  float size = getRadius() * AXIS_FRACTION;
  if (size < AXIS_MIN_M) {
    size = AXIS_MIN_M;
  }
  if (size > AXIS_MAX_M) {
    size = AXIS_MAX_M;
  }
  OkPoint origin(at.x, at.y, at.z);
  for (int axis = 0; axis < 3; axis++) {
    glm::vec4 along(0.0f, 0.0f, 0.0f, 0.0f);
    along[axis]   = size;
    glm::vec4 end = model * glm::vec4(along.x, along.y, along.z, 1.0f);
    addDebugLine(origin, OkPoint(end.x, end.y, end.z), axis == 0 ? 1.0f : 0.0f,
                 axis == 1 ? 1.0f : 0.0f, axis == 2 ? 1.0f : 0.0f);
  }
}

void OkObject::flushDebugHelpers() {
  std::vector<DebugBucket> &buckets = debugBuckets();
  if (buckets.empty()) {
    return;
  }
  GLint program = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &program);
  if (program != 0) {
    GLint modelLoc = glGetUniformLocation(program, "model");
    if (modelLoc != -1) {
      glm::mat4 identity(1.0f);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identity));
    }
    GLint hasTexLoc = glGetUniformLocation(program, "hasTexture");
    if (hasTexLoc != -1) {
      glUniform1i(hasTexLoc, 0);
    }
    GLint colorLoc = glGetUniformLocation(program, "wireframeColor");

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    for (size_t i = 0; i < buckets.size(); i++) {
      if (buckets[i].verts.empty()) {
        continue;
      }
      glBufferData(
          GL_ARRAY_BUFFER,
          static_cast<GLsizeiptr>(buckets[i].verts.size() * sizeof(float)),
          buckets[i].verts.data(), GL_STREAM_DRAW);
      if (colorLoc != -1) {
        glUniform4f(colorLoc, buckets[i].r, buckets[i].g, buckets[i].b, 1.0f);
      }
      glDrawArrays(GL_LINES, 0,
                   static_cast<GLsizei>(buckets[i].verts.size() / 3));
    }
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
  }
  buckets.clear();
}
