#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

OkCamera::OkCamera(float width, float height) : OkObject() {
  // Initialize matrices
  view       = glm::mat4(1.0f);
  projection = glm::mat4(1.0f);

  // Set camera properties
  aspectRatio = width / height;
  fov         = 45.0f;
  near        = 0.1f;
  far         = 100.0f;

  // Create projection matrix
  projection = glm::perspective(glm::radians(fov), aspectRatio, near, far);

  // Initialize direction vectors
  front = OkPoint(0.0f, 0.0f, -1.0f);
  up    = OkPoint(0.0f, 1.0f, 0.0f);

  // Set initial position and update view
  setPosition(0.0f, 0.0f, 3.0f);
  updateView();
}

/**
 * @brief Set the view matrix to a new value.
 *        This method allows you to set a custom view matrix.
 * @param newView The new view matrix.
 */
// void OkCamera::setView(const glm::mat4 &newView) {
//   view = newView;
// }

/**
 * @brief Set the perspective projection matrix.
 *        This method allows you to set a custom perspective projection.
 * @param fovDegrees The field of view in degrees.
 * @param nearPlane  The near clipping plane distance.
 * @param farPlane   The far clipping plane distance.
 */
void OkCamera::setPerspective(float fovDegrees, float nearPlane,
                              float farPlane) {
  fov        = fovDegrees;
  near       = nearPlane;
  far        = farPlane;
  projection = glm::perspective(glm::radians(fov), aspectRatio, near, far);
}

void OkCamera::setDirection(float x, float y, float z) {
  front.setX(x);
  front.setY(y);
  front.setZ(z);
  front = front.normalize();
  updateView();
}

void OkCamera::setDirection(const OkPoint &direction) {
  front = direction.normalize();
  updateView();
}

void OkCamera::updateView() {
  glm::vec3 pos(position.x(), position.y(), position.z());
  glm::vec3 frontVec = front.toVec3();
  glm::vec3 upVec    = up.toVec3();
  view               = glm::lookAt(pos, pos + frontVec, upVec);
}

void OkCamera::updateTransform() {
  // Call parent's updateTransform first
  OkObject::updateTransform();
  // Update the view matrix when transform changes
  updateView();
}
