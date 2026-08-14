#include "ray.hpp"

#include <array>
#include <cmath>
#include <glm/vec4.hpp>

namespace {

  // Below this a number is nothing, and dividing by it turns a distance
  // into an infinity. Used for a direction component that is zero (the
  // ray runs parallel to a pair of planes) and for a determinant that is
  // zero (the ray lies in a triangle's own plane).
  const float NEARLY_ZERO = 1e-9f;

  // The slab test starts with an interval reaching as far as anything can
  // be, and narrows it axis by axis.
  const float REACH_LIMIT = 1e30f;

}  // namespace

OkPoint OkRay::pointAt(float distance) const {
  return origin + direction * distance;
}

bool OkRay::intersectsBox(const OkPoint &low, const OkPoint &high,
                          float *outDistance) const {
  float entry = 0.0f;
  float exit  = REACH_LIMIT;

  const std::array<float, 3> from   = {origin.x(), origin.y(), origin.z()};
  const std::array<float, 3> along  = {direction.x(), direction.y(),
                                       direction.z()};
  const std::array<float, 3> bottom = {low.x(), low.y(), low.z()};
  const std::array<float, 3> top    = {high.x(), high.y(), high.z()};

  for (size_t axis = 0; axis < from.size(); axis++) {
    if (std::fabs(along[axis]) < NEARLY_ZERO) {
      // Parallel to this pair of planes: either always between them or
      // never, and no distance to contribute either way.
      if (from[axis] < bottom[axis] || from[axis] > top[axis]) {
        return false;
      }
      continue;
    }
    float inverse = 1.0f / along[axis];
    float first   = (bottom[axis] - from[axis]) * inverse;
    float second  = (top[axis] - from[axis]) * inverse;
    if (first > second) {
      float swap = first;
      first      = second;
      second     = swap;
    }
    entry = (first > entry) ? first : entry;
    exit  = (second < exit) ? second : exit;
    if (entry > exit) {
      return false;
    }
  }

  if (outDistance != nullptr) {
    *outDistance = entry;
  }
  return true;
}

bool OkRay::intersectsSphere(const OkPoint &centre, float radius,
                             float *outDistance) const {
  OkPoint toOrigin = origin - centre;

  // The quadratic in t for |origin + t * direction - centre| = radius.
  // Written without assuming a unit direction, because a ray moved into
  // an object's local space does not have one.
  float a = direction.dot(direction);
  if (a < NEARLY_ZERO) {
    return false;  // a ray going nowhere
  }
  float b            = 2.0f * toOrigin.dot(direction);
  float c            = toOrigin.dot(toOrigin) - radius * radius;
  float discriminant = b * b - 4.0f * a * c;
  if (discriminant < 0.0f) {
    return false;
  }

  float root    = std::sqrt(discriminant);
  float nearHit = (-b - root) / (2.0f * a);
  float farHit  = (-b + root) / (2.0f * a);
  float entry   = nearHit;
  if (entry < 0.0f) {
    // The near crossing is behind us: either the ray starts inside the
    // sphere (the far one is ahead) or the whole sphere is behind it.
    if (farHit < 0.0f) {
      return false;
    }
    entry = 0.0f;
  }

  if (outDistance != nullptr) {
    *outDistance = entry;
  }
  return true;
}

bool OkRay::intersectsTriangle(const OkPoint &a, const OkPoint &b,
                               const OkPoint &c, float *outDistance) const {
  OkPoint edge1 = b - a;
  OkPoint edge2 = c - a;

  OkPoint across      = direction.cross(edge2);
  float   determinant = edge1.dot(across);
  if (std::fabs(determinant) < NEARLY_ZERO) {
    return false;  // the ray lies in the triangle's plane
  }
  float inverse = 1.0f / determinant;

  // The hit written in the triangle's own coordinates: u along the first
  // edge, v along the second, and inside when both are positive and they
  // sum to no more than one.
  OkPoint toVertex = origin - a;
  float   u        = toVertex.dot(across) * inverse;
  if (u < 0.0f || u > 1.0f) {
    return false;
  }

  OkPoint sideways = toVertex.cross(edge1);
  float   v        = direction.dot(sideways) * inverse;
  if (v < 0.0f || u + v > 1.0f) {
    return false;
  }

  float distance = edge2.dot(sideways) * inverse;
  if (distance <= 0.0f) {
    return false;  // behind the origin
  }

  if (outDistance != nullptr) {
    *outDistance = distance;
  }
  return true;
}

OkRay OkRay::transformed(const glm::mat4 &matrix) const {
  // The origin is a point and moves with the translation (w = 1); the
  // direction is a vector and does not (w = 0). The direction is not
  // renormalized afterwards on purpose -- see the note on the class.
  glm::vec4 movedOrigin =
      matrix * glm::vec4(origin.x(), origin.y(), origin.z(), 1.0f);
  glm::vec4 movedDirection =
      matrix * glm::vec4(direction.x(), direction.y(), direction.z(), 0.0f);

  return OkRay(OkPoint(movedOrigin.x, movedOrigin.y, movedOrigin.z),
               OkPoint(movedDirection.x, movedDirection.y, movedDirection.z));
}
