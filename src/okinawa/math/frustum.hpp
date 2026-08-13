#ifndef OK_FRUSTUM_HPP
#define OK_FRUSTUM_HPP

#include <array>
#include <glm/ext/matrix_float4x4.hpp>

/**
 * @brief View frustum as six planes, for bounding-sphere culling.
 *
 *        Planes are extracted from a combined projection * view matrix with
 *        the Gribb-Hartmann method (each plane is a row combination of the
 *        matrix), stored unnormalized-then-normalized so the plane distance
 *        test works in world units.
 *
 *        The engine keeps one active frustum per frame (set by OkCore from
 *        the current camera before the world pass); OkItem::drawSelf skips
 *        the draw when the item's bounding sphere is fully outside. The GUI
 *        pass disables the test (its calibrated camera is not the world
 *        camera), and the skybox dome is camera-centred so it always
 *        intersects.
 */
class OkFrustum {
public:
  // A frustum is six planes -- left, right, bottom, top, near, far --
  // and each plane is the four coefficients of ax + by + cz + d.
  static const int PLANE_COUNT  = 6;
  static const int PLANE_COEFFS = 4;

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

  // Viewer position and draw distance for the frame. Anything whose
  // bounding sphere lies entirely beyond `maxDistance` is skipped: past
  // a certain range distance fog has swallowed the world anyway, and
  // sending those draws is pure waste. A maxDistance of 0 disables the
  // test. Set by the core from the active camera each frame.
  static void  setViewer(float x, float y, float z, float maxDistance);
  static bool  isBeyondDrawDistance(float x, float y, float z, float radius);
  static float getViewerX();
  static float getViewerY();
  static float getViewerZ();
  // Draws skipped for being too far, this frame.
  static long getDistanceCulledCount();

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
  std::array<std::array<float, PLANE_COEFFS>, PLANE_COUNT> planes;

  static const OkFrustum     *_active;
  static long                 _culled;
  static long                 _drawCalls;
  static long                 _triangles;
  static long                 _distanceCulled;
  static std::array<float, 3> _viewer;
  static float                _maxDistance;
};

#endif
