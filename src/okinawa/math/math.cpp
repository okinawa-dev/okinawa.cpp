#include "math.hpp"
#include "math/point.hpp"
#include "math/rotation.hpp"
#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
// #include <glm/gtx/rotate_vector.hpp>

/**
 * @brief Convert Euler angles to a direction vector.
 *        This method converts pitch, yaw, and roll angles to a direction
 *        vector consistent with the OkRotation class implementation.
 * @param pitch The pitch angle in radians.
 * @param yaw   The yaw angle in radians.
 * @param roll  The roll angle in radians (unused for direction).
 * @return A normalized direction vector as an OkPoint.
 */
// OkPoint OkMath::anglesToDirectionVector(float pitch, float yaw, float roll) {
//   // Calculate direction vector components using the same convention as
//   // OkRotation::getForwardVector
//   float cp = std::cos(pitch);
//   float sp = std::sin(pitch);
//   float cy = std::cos(yaw);
//   float sy = std::sin(yaw);

//   // Forward vector = (sin(yaw)cos(pitch), sin(pitch), cos(yaw)cos(pitch))
//   glm::vec3 direction(sy * cp,  // x component
//                       sp,       // y component
//                       cy * cp   // z component
//   );

//   // Return normalized direction vector
//   return OkPoint(glm::normalize(direction));
// }

/**
 * @brief Convert a direction vector to Euler angles.
 *        This method converts a direction vector to pitch and yaw angles
 *        consistent with the OkRotation class implementation.
 * @param direction The direction vector as an OkPoint.
 * @param outPitch  The output pitch angle in radians.
 * @param outYaw    The output yaw angle in radians.
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

  // Normalize the direction vector to ensure accurate angle calculations
  glm::vec3 normalized = glm::normalize(glm::vec3(x, y, z));
  x                    = normalized.x;
  y                    = normalized.y;
  z                    = normalized.z;

  // Pitch is the elevation; positive y looks up.
  outPitch = asin(y);

  // Vertical look (near ±90° pitch): yaw is indeterminate, default to 0.
  if (std::abs(std::abs(y) - 1.0f) < 0.0001f) {
    outYaw = 0.0f;
  }
  // Inverse of OkRotation::getForwardVector = (-sin(yaw)cos(pitch),
  // sin(pitch), -cos(yaw)cos(pitch)). atan2 is scale-invariant, so the positive
  // cos(pitch) factor drops out and yaw = atan2(-x, -z).
  else {
    outYaw = atan2(-x, -z);
  }

  // This code shouldn't be reached due to early return above
  // else {
  //   // Special case - looking straight up or down
  //   // In this case, yaw becomes arbitrary, so we can maintain the previous
  //   // yaw or set it to a default value. Here we'll use atan2 but be aware
  //   // it may be unstable.
  //   outYaw = atan2(x, -z);
  // }
}

/**
 * @brief Checks if two floating point values are approximately equal
 * @param a First value
 * @param b Second value
 * @param epsilon The maximum difference (default: 1e-6)
 * @return True if values are approximately equal
 */
// static bool approximatelyEqual(float a, float b, float epsilon = 1e-6f) {
//   return fabs(a - b) <= epsilon;
// }

/**
 * @brief Creates a rotation that orients an object to look at a target
 * @param eye The position of the viewer/object
 * @param target The position to look at
 * @param up The up vector that defines the orientation (defaults to world up)
 * @return An OkRotation that will orient an object at 'eye' to face 'target'
 */
OkRotation OkMath::lookAt(const OkPoint &eye, const OkPoint &target,
                          const OkPoint &up) {
  // Direction from eye to target, in the camera's getForwardVector convention
  // (the camera renders from getForwardVector/getUpVector, which depend only on
  // pitch and yaw and ignore roll). So this returns a roll-free orientation;
  // `up` only matters when the forward is vertical (the gimbal-lock case),
  // where it selects which world direction is "up" on screen.
  glm::vec3 forward = glm::normalize(glm::vec3(
      target.x() - eye.x(), target.y() - eye.y(), target.z() - eye.z()));

  float fy = forward.y;
  fy = std::min(fy, 1.0f);
  fy = std::max(fy, -1.0f);
  float pitch = asin(fy);

  const float eps = 1e-4f;
  float       yaw = 0.0f;
  if (std::abs(std::abs(forward.y) - 1.0f) < eps) {
    // Vertical look. getUpVector at vertical is (-sin(yaw), 0, -cos(yaw)) when
    // looking down and (sin(yaw), 0, cos(yaw)) when looking up, so pick yaw to
    // match the horizontal part of `up` (e.g. world +Z for a north-up map).
    glm::vec3 upHorizontal(up.x(), 0.0f, up.z());
    if (glm::length(upHorizontal) >= eps) {
      upHorizontal = glm::normalize(upHorizontal);
      if (forward.y < 0.0f) {
        yaw = atan2(-upHorizontal.x, -upHorizontal.z);
      } else {
        yaw = atan2(upHorizontal.x, upHorizontal.z);
      }
    }
  } else {
    // Inverse of getForwardVector: yaw = atan2(-x, -z).
    yaw = atan2(-forward.x, -forward.z);
  }

  return OkRotation(pitch, yaw, 0.0f);
}
