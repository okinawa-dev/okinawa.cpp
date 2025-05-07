#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

/**
 * @brief Constructor for the OkCamera class.
 *        Initializes the camera with a given width and height.
 * @param width  The width of the viewport.
 * @param height The height of the viewport.
 */
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

  // Initialize direction vectors and angles
  front = OkPoint(0.0f, 0.0f, -1.0f);
  up    = OkPoint(0.0f, 1.0f, 0.0f);
  pitch = 0.0f;
  yaw   = -90.0f;  // -90 degrees to look along negative Z

  // Set initial position and update view
  setPosition(0.0f, 0.0f, 3.0f);
  updateView();
}

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

/**
 * @brief Set the camera direction using an OkPoint.
 *        This method allows you to set the camera's direction using an
 *        OkPoint.
 * @param direction The direction vector as an OkPoint.
 */
void OkCamera::setDirection(const OkPoint &direction) {
  front = direction.normalize();
  OkMath::directionVectorToAngles(front, pitch, yaw);
  updateView();
}

/**
 * @brief Update the view matrix based on the camera's position and direction.
 *        This method recalculates the view matrix using the current position,
 *        front vector, and up vector.
 */
void OkCamera::updateView() {
  glm::vec3 pos(position.x(), position.y(), position.z());
  glm::vec3 frontVec = front.toVec3();
  glm::vec3 upVec    = up.toVec3();
  view               = glm::lookAt(pos, pos + frontVec, upVec);
}

/**
 * @brief Update the camera's transform.
 */
void OkCamera::updateTransform() {
  // Call parent's updateTransform first
  OkObject::updateTransform();
  // Update the view matrix when transform changes
  updateView();
}
