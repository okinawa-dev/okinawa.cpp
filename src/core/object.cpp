#include "object.hpp"

OkObject::OkObject() : position(0.0f, 0.0f, 0.0f), scaling(1.0f, 1.0f, 1.0f) {}

OkObject::~OkObject() {
  detachFromParent();
  detachAllChildren();
}

OkPoint OkObject::getPosition() const {
  if (_parent) {
    // Transform local position by parent's transform
    OkPoint worldPos = _parent->getRotation().transformPoint(position);
    return worldPos + _parent->getPosition();
  }
  return position;
}

void OkObject::setPosition(float x, float y, float z) {
  position = OkPoint(x, y, z);
  updateTransform();
}

void OkObject::move(float dx, float dy, float dz) {
  position = position + OkPoint(dx, dy, dz);
  updateTransform();
}

OkRotation OkObject::getRotation() const {
  if (_parent) {
    return _parent->getRotation().combine(rotation);
  }
  return rotation;
}

void OkObject::setRotation(float x, float y, float z) {
  rotation.setRotation(x, y, z);
  updateTransform();
}

void OkObject::rotate(float dx, float dy, float dz) {
  rotation.rotate(dx, dy, dz);
  updateTransform();
}

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

void OkObject::detachAllChildren() {
  while (_firstChild) {
    _firstChild->detachFromParent();
  }
}

glm::mat4 OkObject::getTransformMatrix() const {
  glm::mat4 matrix(1.0f);

  // Apply parent transform first if exists
  if (_parent) {
    matrix = _parent->getTransformMatrix();
  }

  // Apply local transform: Scale * Rotation * Translation
  matrix = glm::translate(matrix,
                          glm::vec3(position.x(), position.y(), position.z()));
  matrix *= rotation.getMatrix();
  matrix = glm::scale(matrix, glm::vec3(scaling.x(), scaling.y(), scaling.z()));

  return matrix;
}

void OkObject::updateTransform() {
  // Virtual function that derived classes can override to handle transform
  // updates
}
