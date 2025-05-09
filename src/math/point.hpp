#ifndef OK_POINT_HPP
#define OK_POINT_HPP

// if we want to use glm::to_string and do not have a warning, we have to enable
// the experimental features
// #define GLM_ENABLE_EXPERIMENTAL
// #include <glm/gtx/string_cast.hpp>
#include <glm/glm.hpp>
#include <sstream>
#include <string>

class OkPoint {
private:
  glm::vec3 v;  // Internal vec3 array from GLM

public:
  // Constructors
  OkPoint() : v(0.0f) {}
  OkPoint(float x, float y, float z);
  OkPoint(const OkPoint &other) = default;
  explicit OkPoint(const glm::vec3 &vec) : v(vec) {}

  // Operators
  OkPoint &operator=(const OkPoint &other) = default;
  bool     operator==(const OkPoint &other) const { return v == other.v; }
  OkPoint operator+(const OkPoint &other) const { return OkPoint(v + other.v); }
  OkPoint operator-(const OkPoint &other) const { return OkPoint(v - other.v); }
  OkPoint operator*(float scalar) const { return OkPoint(v * scalar); }
  OkPoint operator-() const;
  OkPoint &operator+=(const OkPoint &other);
  OkPoint &operator-=(const OkPoint &other);
  OkPoint &operator*=(float scalar);

  // Methods
  float   magnitude() const;
  OkPoint normalize() const;
  float   distance(const OkPoint &destination) const;

  // Vector operations
  float   dot(const OkPoint &other) const;
  OkPoint cross(const OkPoint &other) const;

  // Getters/Setters
  float x() const { return v.x; }
  float y() const { return v.y; }
  float z() const { return v.z; }
  void  setX(float x) { v.x = x; }
  void  setY(float y) { v.y = y; }
  void  setZ(float z) { v.z = z; }

  // GLM data access
  const glm::vec3 &data() const { return v; }
  glm::vec3       &data() { return v; }

  // String representation
  std::string toString() const;

  // Conversion
  glm::vec3 toVec3() const { return v; }
};

#endif  // OK_POINT_HPP
