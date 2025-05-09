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

  // static OkPoint anglesToDirectionVector(float pitch, float yaw, float roll);
  static void directionVectorToAngles(const OkPoint &direction, float &outPitch,
                                      float &outYaw);

  static OkRotation lookAt(const OkPoint &eye, const OkPoint &target,
                           const OkPoint &up = OkPoint(0, 1, 0));

  // static bool approximatelyEqual(float a, float b, float epsilon = 1e-6f);
};

#endif
