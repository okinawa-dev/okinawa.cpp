#include "math.hpp"
#include "math/point.hpp"
#include "math/rotation.hpp"
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

  // Calculate pitch (up/down)
  outPitch = asin(y);
  float cp = cos(outPitch);

  // Handle vertical look case first (near ±90° pitch)
  if (std::abs(std::abs(y) - 1.0f) < 0.0001f) {
    outYaw = 0.0f;  // Set default yaw for vertical look
  }
  // this second else-if is an impossible case since we already checked
  // for vertical look above
  // Handle near-vertical case where cos(pitch) is very small
  // else if (cp <= 0.001f) {
  //   outYaw = 0.0f;  // Set default yaw for near-vertical look
  // }
  // Normal case - calculate yaw
  else {
    outYaw = atan2(x / cp, -z / cp);
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
  // Calculate forward direction (from eye to target)
  glm::vec3 eyePos(eye.x(), eye.y(), eye.z());
  glm::vec3 targetPos(target.x(), target.y(), target.z());
  glm::vec3 worldUp(up.x(), up.y(), up.z());

  // Normalize the up vector
  worldUp = glm::normalize(worldUp);

  // Calculate the forward vector (direction from eye to target)
  glm::vec3 forward = glm::normalize(targetPos - eyePos);

  // Special case: if forward and up are parallel, nudge the up vector
  if (glm::abs(glm::dot(forward, worldUp)) > 0.999999f) {
    // Choose a different up vector if they're nearly parallel
    if (glm::abs(forward.y) < 0.999999f) {
      worldUp = glm::vec3(0, 0, 1);  // Use world Z as up instead
    } else {
      worldUp = glm::vec3(1, 0, 0);  // Use world X as up instead
    }
  }

  // Calculate the right vector
  glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));

  // Recalculate the orthogonal up vector
  glm::vec3 upDir = glm::cross(right, forward);

  // Extract the rotation angles from these vectors

  // 1. Calculate pitch: angle between forward and the xz-plane
  // The dot product of forward and (0,1,0) gives us the sine of the pitch angle
  float pitch = -asin(forward.y);

  // Check for vertical look (near ±90° pitch)
  if (std::abs(std::abs(forward.y) - 1.0f) < 0.001f) {
    // Looking straight up or down
    return OkRotation(pitch, 0.0f, 0.0f);  // Set yaw and roll to 0
  }

  // 2. Calculate yaw: angle between the projection of forward on the xz-plane
  // and the z-axis Project forward onto xz-plane by zeroing y component
  glm::vec3 forwardXZ = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
  float     yaw       = atan2(forwardXZ.x, forwardXZ.z);

  // 3. Calculate roll (only for non-vertical orientations): measure how much
  // the up vector is rotated around the forward axis First, create the
  // "expected" up vector for this pitch and yaw (without roll)
  float cp = cos(pitch);
  float sp = sin(pitch);
  float cy = cos(yaw);
  float sy = sin(yaw);

  // Expected up vector if roll=0 (derived from rotation matrix)
  glm::vec3 expectedUp = glm::normalize(glm::vec3(-sy * sp, cp, -cy * sp));

  // Project actual up vector onto the plane perpendicular to forward
  glm::vec3 projectedUp = upDir - forward * glm::dot(upDir, forward);
  projectedUp           = glm::normalize(projectedUp);

  // Project expected up onto the same plane
  glm::vec3 projectedExpectedUp =
      expectedUp - forward * glm::dot(expectedUp, forward);
  projectedExpectedUp = glm::normalize(projectedExpectedUp);

  // Calculate roll by finding the angle between these two projections
  float roll =
      acos(glm::clamp(glm::dot(projectedUp, projectedExpectedUp), -1.0f, 1.0f));

  // Determine roll sign by testing which side of the expected up the actual up
  // is
  glm::vec3 rollTest = glm::cross(projectedExpectedUp, projectedUp);
  if (glm::dot(rollTest, forward) < 0) {
    roll = -roll;
  }

  return OkRotation(pitch, yaw, roll);
}
