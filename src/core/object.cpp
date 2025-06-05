#include "object.hpp"
#include "../config/config.hpp"
#include "gl_config.hpp"
#include <glm/gtc/type_ptr.hpp>

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
  // OkLogger::info("Drawing object in hierarchy, name: " + name);

  // Call the derived class's specific drawing logic
  drawSelf();

  drawAxis();

  // Draw children recursively (this stays in OkObject)
  OkObject *current = _firstChild;
  while (current != nullptr) {
    current->draw();
    current = current->getNextSibling();
  }
}

/**
 * @brief Draw coordinate axis for this object using its transform matrix.
 *        Shows X (red), Y (green), and Z (blue) axes using simple OpenGL.
 */
void OkObject::drawAxis() const {
  // Create axis line vertices in local space (origin-based)
  float axisVertices[] = {// X-axis (red) - 100 units long
                          0.0f, 0.0f, 0.0f, 100.0f, 0.0f, 0.0f,

                          // Y-axis (green) - 100 units long
                          0.0f, 0.0f, 0.0f, 0.0f, 100.0f, 0.0f,

                          // Z-axis (blue) - 100 units long
                          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 100.0f};

  // Create temporary VAO and VBO for the axis lines
  GLuint VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices,
               GL_STATIC_DRAW);

  // Configure vertex attributes (position only at location 0)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // Get current shader program to reuse it
  GLint current_program;
  glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);

  if (current_program != 0) {
    // Get model matrix uniform location
    GLint modelLoc = glGetUniformLocation(current_program, "model");
    if (modelLoc != -1) {
      glm::mat4 model = getTransformMatrix();
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    }

    // Try to disable texturing
    GLint hasTexLoc = glGetUniformLocation(current_program, "hasTexture");
    if (hasTexLoc != -1) {
      glUniform1i(hasTexLoc, 0);
    }

    // Draw each axis with different colors
    GLint colorLoc = glGetUniformLocation(current_program, "wireframeColor");

    // Draw X-axis in red
    if (colorLoc != -1) {
      glUniform4f(colorLoc, 1.0f, 0.0f, 0.0f, 1.0f);
    }
    glDrawArrays(GL_LINES, 0, 2);

    // Draw Y-axis in green
    if (colorLoc != -1) {
      glUniform4f(colorLoc, 0.0f, 1.0f, 0.0f, 1.0f);
    }
    glDrawArrays(GL_LINES, 2, 2);

    // Draw Z-axis in blue
    if (colorLoc != -1) {
      glUniform4f(colorLoc, 0.0f, 0.0f, 1.0f, 1.0f);
    }
    glDrawArrays(GL_LINES, 4, 2);
  }

  // Clean up temporary buffers
  glBindVertexArray(0);
  glDeleteBuffers(1, &VBO);
  glDeleteVertexArrays(1, &VAO);
}
