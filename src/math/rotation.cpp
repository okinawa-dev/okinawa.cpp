#include "rotation.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <sstream>

/**
 * @brief Default constructor initializes rotation to identity.
 * The rotation matrix is set to the identity matrix and angles are set to zero.
 */
OkRotation::OkRotation() : matrix(1.0f), angles(0.0f) {}

/**
 * @brief Constructor to initialize rotation with specified angles.
 * @param pitch Rotation around X axis in radians.
 * @param yaw   Rotation around Y axis in radians.
 * @param roll  Rotation around Z axis in radians.
 */
OkRotation::OkRotation(float pitch, float yaw, float roll)
    : angles(pitch, yaw, roll) {
  _updateMatrix();
}

// 3D Rotation matrices for each axis:
//
// X rotation (Pitch):
//   | 1    0      0    |
//   | 0  cos(x) -sin(x)|
//   | 0  sin(x)  cos(x)|
//
// Y rotation (Yaw):
//   |  cos(y)  0  sin(y)|
//   |    0     1    0   |
//   | -sin(y)  0  cos(y)|
//
// Z rotation (Roll):
//   | cos(z) -sin(z)  0 |
//   | sin(z)  cos(z)  0 |
//   |   0       0     1 |
//
// Combined rotation matrix (ZYX order):
//   | cy*cz  cz*sx*sy-cx*sz  cx*cz*sy+sx*sz |
//   | cy*sz  cx*cz+sx*sy*sz  -cz*sx+cx*sy*sz|
//   |  -sy      cy*sx            cx*cy      |
//
// Where: cx = cos(x), sx = sin(x), etc.

/**
 * @brief Update the rotation matrix based on the current angles.
 * This function computes the rotation matrix using the ZYX order of
 * rotations (yaw, pitch, roll).
 */
void OkRotation::_updateMatrix() {
  // Reset matrix to identity
  matrix = glm::mat4(1.0f);

  // experimental GLM feature
  // Apply rotations in ZYX order using GLM
  // matrix = glm::eulerAngleXYZ(angles.x, angles.y, angles.z);

  // Get angles for readability
  float pitch = angles.x;  // X rotation
  float yaw   = angles.y;  // Y rotation
  float roll  = angles.z;  // Z rotation

  // Precompute trigonometric values
  float cp = std::cos(pitch);
  float sp = std::sin(pitch);
  float cy = std::cos(yaw);
  float sy = std::sin(yaw);
  float cr = std::cos(roll);
  float sr = std::sin(roll);

  // Build rotation matrix (ZYX order)
  // clang-format off
  matrix = glm::mat4(
      cy * cr,                   -cy * sr,                 sy,        0.0f,
      cp * sr + cr * sp * sy,    cp * cr - sp * sy * sr,  -cy * sp,  0.0f,
      sp * sr - cp * cr * sy,    cr * sp + cp * sy * sr,   cp * cy,  0.0f,
      0.0f,                      0.0f,                     0.0f,      1.0f
  );
  // clang-format on
}

/**
 * @brief Rotate the object by specified angles.
 * @param dx Rotation around X axis in radians.
 * @param dy Rotation around Y axis in radians.
 * @param dz Rotation around Z axis in radians.
 */
void OkRotation::rotate(float dx, float dy, float dz) {
  angles.x += dx;
  angles.y += dy;
  angles.z += dz;
  _updateMatrix();
}

/**
 * @brief Set the rotation angles.
 * @param x Rotation around X axis in radians.
 * @param y Rotation around Y axis in radians.
 * @param z Rotation around Z axis in radians.
 */
void OkRotation::setRotation(float x, float y, float z) {
  angles = glm::vec3(x, y, z);
  _updateMatrix();
}

/**
 * @brief Transform a point using the rotation matrix.
 * @param point The point to transform.
 * @return The transformed point.
 */
OkPoint OkRotation::transformPoint(const OkPoint &point) const {
  glm::vec4 temp = matrix * glm::vec4(point.x(), point.y(), point.z(), 1.0f);
  return OkPoint(temp.x, temp.y, temp.z);
}

/**
 * @brief Combine this rotation with another rotation.
 * @param other The rotation to combine with.
 * @return A new rotation that represents both rotations applied in sequence.
 * @note The resulting rotation is equivalent to applying this rotation first,
 *       then applying the other rotation.
 */
OkRotation OkRotation::combine(const OkRotation &other) const {
  // Get the matrices for both rotations
  glm::mat4 thisMatrix = glm::eulerAngleXYZ(angles.x, angles.y, angles.z);
  glm::mat4 otherMatrix =
      glm::eulerAngleXYZ(other.angles.x, other.angles.y, other.angles.z);

  // Combine matrices (multiply in correct order)
  glm::mat4 combined = otherMatrix * thisMatrix;

  // Extract Euler angles from the combined matrix
  glm::vec3 combinedAngles;
  glm::extractEulerAngleXYZ(combined, combinedAngles.x, combinedAngles.y,
                            combinedAngles.z);

  return OkRotation(combinedAngles.x, combinedAngles.y, combinedAngles.z);
}

bool OkRotation::operator==(const OkRotation &other) const {
  return angles == other.angles;
}

/**
 * @brief Convert the rotation to a string representation.
 * @return A string representation of the rotation.
 */
std::string OkRotation::toString() const {
  std::stringstream ss;
  ss << "(" << angles.x << ", " << angles.y << ", " << angles.z << ")";
  return ss.str();
}
