#ifndef OK_ROTATION_HPP
#define OK_ROTATION_HPP

#include "point.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <string>

class OkRotation {
private:
  glm::mat4 matrix;  // 4x4 transformation matrix
  glm::vec3 angles;  // Euler angles in radians (x=pitch, y=yaw, z=roll)

  // Update matrix based on angles
  void _updateMatrix();

public:
  // Constructors
  OkRotation();
  OkRotation(float pitch, float yaw, float roll);
  OkRotation(const OkRotation &other) = default;

  // Getters
  const glm::mat4 &getMatrix() const { return matrix; }
  const glm::vec3 &getAngles() const { return angles; }

  float getPitch() const;
  float getYaw() const;
  float getRoll() const;

  // Setters
  void rotate(float dx, float dy, float dz);
  void setRotation(float x, float y, float z);

  // Transform methods
  OkPoint    transformPoint(const OkPoint &point) const;
  OkRotation combine(const OkRotation &other) const;

  // Operators
  OkRotation &operator=(const OkRotation &other) = default;
  bool        operator==(const OkRotation &other) const;

  // String representation
  std::string toString() const;

  // Direction vectors
  OkPoint getForwardVector() const;
  OkPoint getRightVector() const;
  OkPoint getUpVector() const;
};

#endif
