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

/**
 * @brief Update the rotation matrix based on the current angles.
 * This function computes the rotation matrix using the ZYX order of
 * rotations (yaw, pitch, roll).
 */
void OkRotation::_updateMatrix() {
  // Reset matrix to identity
  matrix = glm::mat4(1.0f);

  // Get angles for readability
  float pitch = angles.x;  // X rotation (looking up/down)
  float yaw   = angles.y;  // Y rotation (looking left/right)
  float roll  = angles.z;  // Z rotation (tilting head)

  // Precompute trigonometric values
  float cp = std::cos(pitch);
  float sp = std::sin(pitch);
  float cy = std::cos(yaw);
  float sy = std::sin(yaw);
  float cr = std::cos(roll);
  float sr = std::sin(roll);

  // Build rotation matrix (YXZ order - yaw, then pitch, then roll)
  // This matches standard FPS camera systems
  // clang-format off
  matrix = glm::mat4(
      cy * cr + sy * sp * sr,    -cy * sr + sy * sp * cr,    sy * cp,    0.0f,
      cp * sr,                    cp * cr,                   -sp,        0.0f,
      -sy * cr + cy * sp * sr,    sy * sr + cy * sp * cr,    cy * cp,    0.0f,
      0.0f,                       0.0f,                      0.0f,       1.0f
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
  // Simply multiply the matrices
  glm::mat4 combined = other.matrix * matrix;

  // The combined rotation needs to be converted back to Euler angles
  // This is a complex process because extracting Euler angles from a rotation
  // matrix can have multiple solutions (gimbal lock issue)

  // For simplicity and consistency with our conventions, we'll extract the
  // key vectors and compute angles from them

  // Extract the main axes from the combined matrix
  glm::vec3 forward(combined[2][0], combined[2][1], combined[2][2]);
  glm::vec3 up(combined[1][0], combined[1][1], combined[1][2]);

  // Normalize to ensure we have unit vectors
  forward = glm::normalize(forward);
  up      = glm::normalize(up);

  // Calculate pitch from forward vector (elevation angle)
  float pitch = -asin(forward.y);

  // Calculate yaw from forward vector (azimuth angle)
  float yaw = atan2(forward.x, forward.z);

  // Calculate roll by finding the angle between the up vector and the expected
  // up vector after applying pitch and yaw (but before roll)

  // Create a rotation matrix for just pitch and yaw
  glm::mat4 pitchYawMatrix = glm::mat4(1.0f);
  float     cp             = cos(pitch);
  float     sp             = sin(pitch);
  float     cy             = cos(yaw);
  float     sy             = sin(yaw);

  // For no-roll orientation, the up vector would be:
  glm::vec3 noRollUp(-sy * sp, cp, -cy * sp);
  noRollUp = glm::normalize(noRollUp);

  // Find the angle between the actual up and the no-roll up
  // projected onto the plane perpendicular to forward
  glm::vec3 right = glm::cross(noRollUp, forward);
  float     roll  = atan2(glm::dot(up, right), glm::dot(up, noRollUp));

  return OkRotation(pitch, yaw, roll);
}

/**
 * @brief Equality operator to compare two rotations.
 * @param other The other rotation to compare with.
 * @return True if the rotations are equal, false otherwise.
 */
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

/**
 * @brief Get the forward vector based on the rotation.
 * @return The forward vector as an OkPoint.
 * @note The forward vector is calculated using the angles of the rotation.
 */
OkPoint OkRotation::getForwardVector() const {
  // Forward vector = (sin(yaw)cos(pitch), sin(pitch), cos(yaw)cos(pitch))
  float pitch = angles.x;
  float yaw   = angles.y;
  float cp    = std::cos(pitch);
  float sp    = std::sin(pitch);
  float cy    = std::cos(yaw);
  float sy    = std::sin(yaw);

  return OkPoint(-sy * cp, sp, -cy * cp);
}

/**
 * @brief Get the right vector based on the rotation.
 * @return The right vector as an OkPoint.
 * @note The right vector is calculated using the yaw angle of the rotation.
 */
OkPoint OkRotation::getRightVector() const {
  // Right vector = (cos(yaw), 0, -sin(yaw))
  float yaw = angles.y;
  float cy  = std::cos(yaw);
  float sy  = std::sin(yaw);

  return OkPoint(cy, 0.0f, -sy);
}

/**
 * @brief Get the up vector based on the rotation.
 * @return The up vector as an OkPoint.
 * @note The up vector is calculated as the cross product of the right and
 *       forward vectors.
 */
OkPoint OkRotation::getUpVector() const {
  // We can calculate the up vector as the cross product of right and forward
  // Up = Right × Forward
  return getRightVector().cross(getForwardVector());
}
