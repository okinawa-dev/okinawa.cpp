#include "math.hpp"
#include <glm/gtx/rotate_vector.hpp>

OkPoint OkMath::anglesToDirectionVector(float pitch, float yaw, float roll) {
  // Convert angles to direction vector using GLM
  glm::vec3 direction(cos(yaw) * cos(pitch),  // x component
                      sin(pitch),             // y component
                      sin(yaw) * cos(pitch)   // z component
  );

  // Normalize using GLM
  return OkPoint(glm::normalize(direction));
}

OkPoint OkMath::getForwardVector(const OkRotation &rotation) {
  const auto &angles = rotation.getAngles();
  float       yaw    = angles.y;  // Y rotation
  float       pitch  = angles.x;  // X rotation

  glm::vec3 forward(cos(yaw) * cos(pitch), -sin(pitch), sin(yaw) * cos(pitch));

  return OkPoint(forward);
}

OkPoint OkMath::getRightVector(const OkRotation &rotation) {
  const auto &angles = rotation.getAngles();
  float       yaw    = angles.y;

  glm::vec3 right(cos(yaw + glm::half_pi<float>()), 0.0f,
                  sin(yaw + glm::half_pi<float>()));

  return OkPoint(right);
}

OkPoint OkMath::getUpVector(const OkRotation &rotation) {
  // Get forward and right vectors
  OkPoint forward = getForwardVector(rotation);
  OkPoint right   = getRightVector(rotation);

  // Calculate cross product using GLM
  glm::vec3 up = glm::cross(glm::vec3(forward.x(), forward.y(), forward.z()),
                            glm::vec3(right.x(), right.y(), right.z()));

  return OkPoint(up);
}
