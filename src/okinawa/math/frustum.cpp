#include "frustum.hpp"
#include <cmath>

const OkFrustum *OkFrustum::_active         = nullptr;
long             OkFrustum::_culled         = 0;
long             OkFrustum::_drawCalls      = 0;
long             OkFrustum::_triangles      = 0;
long             OkFrustum::_distanceCulled = 0;
float            OkFrustum::_viewer[3]      = {0.0f, 0.0f, 0.0f};
float            OkFrustum::_maxDistance    = 0.0f;

OkFrustum::OkFrustum() {
  for (int i = 0; i < 6; i++) {
    planes[i][0] = 0.0f;
    planes[i][1] = 0.0f;
    planes[i][2] = 0.0f;
    planes[i][3] = 0.0f;
  }
}

/**
 * @brief Gribb-Hartmann plane extraction: with row vectors r0..r3 of the
 *        combined matrix, the planes are r3 +/- r0 (left/right), r3 +/- r1
 *        (bottom/top) and r3 +/- r2 (near/far). Each plane is normalized
 *        so containsSphere can compare distances against a world radius.
 */
void OkFrustum::setFromMatrix(const glm::mat4 &projView) {
  // glm is column-major: m[col][row]; row i component of column c is
  // projView[c][i].
  for (int i = 0; i < 6; i++) {
    int   row  = i / 2;  // 0 = x, 1 = y, 2 = z
    float sign = (i % 2 == 0) ? 1.0f : -1.0f;
    for (int c = 0; c < 4; c++) {
      float r3 = projView[c][3];
      float ri = projView[c][row];
      float v  = r3 + sign * ri;
      if (c < 3) {
        planes[i][c] = v;
      } else {
        planes[i][3] = v;
      }
    }
    float len =
        std::sqrt(planes[i][0] * planes[i][0] + planes[i][1] * planes[i][1] +
                  planes[i][2] * planes[i][2]);
    if (len > 1e-9f) {
      planes[i][0] /= len;
      planes[i][1] /= len;
      planes[i][2] /= len;
      planes[i][3] /= len;
    }
  }
}

bool OkFrustum::containsSphere(float x, float y, float z, float radius) const {
  for (int i = 0; i < 6; i++) {
    float d =
        planes[i][0] * x + planes[i][1] * y + planes[i][2] * z + planes[i][3];
    if (d < -radius) {
      return false;
    }
  }
  return true;
}

void OkFrustum::setActive(const OkFrustum *frustum) {
  _active = frustum;
}

const OkFrustum *OkFrustum::getActive() {
  return _active;
}

long OkFrustum::getCulledCount() {
  return _culled;
}

void OkFrustum::resetStats() {
  _culled         = 0;
  _drawCalls      = 0;
  _triangles      = 0;
  _distanceCulled = 0;
}

void OkFrustum::setViewer(float x, float y, float z, float maxDistance) {
  _viewer[0]   = x;
  _viewer[1]   = y;
  _viewer[2]   = z;
  _maxDistance = maxDistance;
}

float OkFrustum::getViewerX() {
  return _viewer[0];
}
float OkFrustum::getViewerY() {
  return _viewer[1];
}
float OkFrustum::getViewerZ() {
  return _viewer[2];
}

/**
 * @brief True when a bounding sphere lies entirely past the draw
 *        distance. The radius counts, so a large object stays visible
 *        while any part of it is within range.
 */
bool OkFrustum::isBeyondDrawDistance(float x, float y, float z, float radius) {
  if (_maxDistance <= 0.0f) {
    return false;
  }
  float dx    = x - _viewer[0];
  float dy    = y - _viewer[1];
  float dz    = z - _viewer[2];
  float limit = _maxDistance + radius;
  return (dx * dx + dy * dy + dz * dz) > (limit * limit);
}

long OkFrustum::getDistanceCulledCount() {
  return _distanceCulled;
}

long OkFrustum::getDrawCalls() {
  return _drawCalls;
}

long OkFrustum::getTriangles() {
  return _triangles;
}

void OkFrustum::addDraw(long triangles) {
  _drawCalls++;
  _triangles += triangles;
}

void OkFrustum::addCulled() {
  _culled++;
}
