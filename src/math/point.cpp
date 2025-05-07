#include "point.hpp"
#include <glm/geometric.hpp>
#include <sstream>

/**
 * @brief Constructor for OkPoint class.
 * @param x The x-coordinate of the point.
 * @param y The y-coordinate of the point.
 * @param z The z-coordinate of the point.
 */
OkPoint::OkPoint(float x, float y, float z) : v(x, y, z) {}

/**
 * @brief Negate the point.
 * @return A new OkPoint object with negated coordinates.
 */
OkPoint OkPoint::operator-() const {
  return OkPoint(-v.x, -v.y, -v.z);
}

/**
 * @brief Add two OkPoint objects.
 * @param other The other OkPoint object to add.
 * @return A new OkPoint object with the sum of the two points.
 */
OkPoint &OkPoint::operator+=(const OkPoint &other) {
  v += other.v;
  return *this;
}

/**
 * @brief Subtract two OkPoint objects.
 * @param other The other OkPoint object to subtract.
 * @return A new OkPoint object with the difference of the two points.
 */
OkPoint &OkPoint::operator-=(const OkPoint &other) {
  v -= other.v;
  return *this;
}

/**
 * @brief Scale the OkPoint by a scalar.
 * @param scalar The scalar value to multiply with.
 * @return A new OkPoint object with the scaled coordinates.
 */
OkPoint &OkPoint::operator*=(float scalar) {
  v *= scalar;
  return *this;
}

/**
 * @brief Calculate the magnitude of the vector.
 * @return The magnitude of the vector.
 */
float OkPoint::magnitude() const {
  return glm::length(v);
}

/**
 * @brief Normalize the vector.
 * @return A new OkPoint object with the normalized vector.
 */
OkPoint OkPoint::normalize() const {
  return OkPoint(glm::normalize(v));
}

/**
 * @brief Calculate the distance between two points.
 * @param destination The destination point.
 * @return The distance between the two points.
 */
float OkPoint::distance(const OkPoint &destination) const {
  return glm::distance(v, destination.v);
}

/**
 * @brief Convert the point to a string representation.
 * @return A string representation of the point.
 */
std::string OkPoint::toString() const {
  std::stringstream ss;
  ss << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return ss.str();
}
