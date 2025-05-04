#ifndef OK_MATH_HPP
#define OK_MATH_HPP

#define GLM_ENABLE_EXPERIMENTAL

#include "point.hpp"
#include "rotation.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class OkMath {
public:
  // Static class - no instantiation
  OkMath() = delete;

  // Convert Euler angles to direction vector
  static OkPoint anglesToDirectionVector(float pitch, float yaw, float roll);

  // Movement vectors
  static OkPoint getForwardVector(const OkRotation &rotation);
  static OkPoint getRightVector(const OkRotation &rotation);
  static OkPoint getUpVector(const OkRotation &rotation);

private:
};

#endif
