#ifndef OK_FRUSTUM_HPP
#define OK_FRUSTUM_HPP

#include <glm/ext/matrix_float4x4.hpp>

/**
 * @brief View frustum as six planes, for bounding-sphere culling.
 *
 *        Planes are extracted from a combined projection * view matrix with
 *        the Gribb-Hartmann method (each plane is a row combination of the
 *        matrix), stored unnormalized-then-normalized so the plane distance
 *        test works in world units.
 *
 *        The engine keeps ONE active frustum per frame (set by OkCore from
 *        the current camera before the world pass); OkItem::drawSelf skips
 *        the draw when the item's bounding sphere is fully outside. The GUI
 *        pass disables the test (its calibrated camera is not the world
 *        camera), and the skybox dome is camera-centred so it always
 *        intersects.
 */
class OkFrustum {
public:
  OkFrustum();

  // Extract the six planes from projection * view.
  void setFromMatrix(const glm::mat4 &projView);

  // True when a sphere at (x, y, z) with the given radius is at least
  // partially inside the frustum.
  bool containsSphere(float x, float y, float z, float radius) const;

  // Global per-frame frustum used by the draw path. `enabled` is false
  // outside the world pass (GUI) so nothing is culled there.
  static void             setActive(const OkFrustum *frustum);
  static const OkFrustum *getActive();

  // Draws skipped by the sphere test since the last resetStats() call.
  static long getCulledCount();
  static void resetStats();
  static void addCulled();

  // Per-frame render counters, reset alongside the culling stats: draw
  // calls actually issued and triangles submitted. They are what tells
  // a project whether a change cost what it thought it would.
  static long getDrawCalls();
  static long getTriangles();
  static void addDraw(long triangles);

private:
  // plane i: ax + by + cz + d, inside when >= -radius
  float planes[6][4];

  static const OkFrustum *_active;
  static long             _culled;
  static long             _drawCalls;
  static long             _triangles;
};

#endif
