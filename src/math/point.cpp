#include "point.hpp"
#include <glm/geometric.hpp>
#include <sstream>

OkPoint::OkPoint(float x, float y, float z) : v(x, y, z) {}

OkPoint OkPoint::operator-() const {
  return OkPoint(-v.x, -v.y, -v.z);
}

OkPoint &OkPoint::operator+=(const OkPoint &other) {
  v += other.v;
  return *this;
}

OkPoint &OkPoint::operator-=(const OkPoint &other) {
  v -= other.v;
  return *this;
}

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
