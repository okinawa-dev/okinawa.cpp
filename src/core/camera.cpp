#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

OkCamera::OkCamera(float width, float height) {
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

  // Create view matrix and move camera back
  view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
}

/**
 * @brief Set the view matrix to a new value.
 *        This method allows you to set a custom view matrix.
 * @param newView The new view matrix.
 */
void OkCamera::setView(const glm::mat4 &newView) {
  view = newView;
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
