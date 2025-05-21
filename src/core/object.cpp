#include "object.hpp"

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
}

/**
 * @brief Destructor for the OkObject class.
 *        Cleans up the object and detaches from parent.
 */
OkObject::~OkObject() {
  detachFromParent();
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
void OkObject::detachFromParent() {
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
