#include "math.hpp"
#include <glm/gtx/rotate_vector.hpp>

/**
 * @brief Convert Euler angles to a direction vector.
 *        This method converts pitch, yaw, and roll angles to a direction
 *        vector using GLM.
 * @param pitch The pitch angle in degrees.
 * @param yaw   The yaw angle in degrees.
 * @param roll  The roll angle in degrees.
 * @return A normalized direction vector as an OkPoint.
 */
OkPoint OkMath::anglesToDirectionVector(float pitch, float yaw, float roll) {
  // Convert angles to direction vector using GLM
  glm::vec3 direction(
      cos(glm::radians(yaw)) * cos(glm::radians(pitch)),  // x component
      sin(glm::radians(pitch)),                           // y component
      sin(glm::radians(yaw)) * cos(glm::radians(pitch))   // z component
  );

  // Normalize using GLM
  return OkPoint(glm::normalize(direction));
}

/**
 * @brief Convert a direction vector to Euler angles.
 *        This method converts a direction vector to pitch and yaw angles
 *        using GLM.
 * @param direction The direction vector as an OkPoint.
 * @param outPitch  The output pitch angle in degrees.
 * @param outYaw    The output yaw angle in degrees.
 * @note There is no roll in this conversion, a single direction vector doesn't
 * contain enough information to determine roll. Roll is a rotation around the
 * direction vector itself, so it cannot be extracted from just the direction
 * vector.
 */
void OkMath::directionVectorToAngles(const OkPoint &direction, float &outPitch,
                                     float &outYaw) {
  // Extract components
  float x = direction.x();
  float y = direction.y();
  float z = direction.z();

  // Calculate pitch (up/down) - convert to degrees
  outPitch = glm::degrees(asin(y));

  // Calculate yaw (left/right) - convert to degrees
  outYaw = glm::degrees(atan2(z, x));
}

/**
 * @brief Get the forward vector based on the rotation.
 *        This method calculates the forward vector using the rotation angles.
 * @param rotation The rotation as an OkRotation.
 * @return The forward vector as an OkPoint.
 */
OkPoint OkMath::getForwardVector(const OkRotation &rotation) {
  const auto &angles = rotation.getAngles();
  float       yaw    = angles.y;  // Y rotation
  float       pitch  = angles.x;  // X rotation

  glm::vec3 forward(cos(yaw) * cos(pitch), -sin(pitch), sin(yaw) * cos(pitch));

  return OkPoint(forward);
}

/**
 * @brief Get the right vector based on the rotation.
 *        This method calculates the right vector using the rotation angles.
 * @param rotation The rotation as an OkRotation.
 * @return The right vector as an OkPoint.
 */
OkPoint OkMath::getRightVector(const OkRotation &rotation) {
  const auto &angles = rotation.getAngles();
  float       yaw    = angles.y;

  glm::vec3 right(cos(yaw + glm::half_pi<float>()), 0.0f,
                  sin(yaw + glm::half_pi<float>()));

  return OkPoint(right);
}

/**
 * @brief Get the up vector based on the rotation.
 *        This method calculates the up vector using the rotation angles.
 * @param rotation The rotation as an OkRotation.
 * @return The up vector as an OkPoint.
 */
OkPoint OkMath::getUpVector(const OkRotation &rotation) {
  // Get forward and right vectors
  OkPoint forward = getForwardVector(rotation);
  OkPoint right   = getRightVector(rotation);

  // Calculate cross product using GLM
  glm::vec3 up = glm::cross(glm::vec3(forward.x(), forward.y(), forward.z()),
                            glm::vec3(right.x(), right.y(), right.z()));

  return OkPoint(up);
}
