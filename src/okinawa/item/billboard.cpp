#include "billboard.hpp"

#include "../core/core.hpp"
#include <cmath>

// Quad geometry shared with the OkItem base constructor. OkItem copies
// the arrays, so returning pointers to these static scratch buffers is
// safe (the engine constructs items from the main thread only).
static float *_quadVertexData(float width, float height) {
  static float verts[20];
  float        hw = width * 0.5f;
  float        hh = height * 0.5f;
  // x, y, z, u, v (stride 5); the visible face looks toward local +Z
  float data[20] = {-hw, -hh, 0.0f, 0.0f, 0.0f,   //
                    hw,  -hh, 0.0f, 1.0f, 0.0f,   //
                    hw,  hh,  0.0f, 1.0f, 1.0f,   //
                    -hw, hh,  0.0f, 0.0f, 1.0f};  //
  for (int i = 0; i < 20; i++) {
    verts[i] = data[i];
  }
  return verts;
}

static unsigned int *_quadIndexData() {
  static unsigned int idx[6] = {0, 1, 2, 0, 2, 3};
  return idx;
}

OkBillboard::OkBillboard(const std::string &name, float width, float height)
    : OkItem(name, _quadVertexData(width, height), 20, _quadIndexData(), 6) {
  proximityFade = 0.0f;
  baseAlpha     = 1.0f;
}

/**
 * @brief Re-orient the quad toward the active camera.
 *
 * View-plane alignment: copying the camera's pitch and yaw points the
 * quad's +Z straight back at the camera plane (the +Z axis of a
 * rotation equals the negated forward vector of the same angles) and
 * keeps the content aligned with the screen at every angle.
 * Position-based facing turns degenerate right under a top-down
 * camera: the horizontal component vanishes and the roll of the label
 * becomes arbitrary. Runs every frame; the camera has already been
 * stepped for this frame when the scene steps its objects.
 */
void OkBillboard::stepSelf(float dt) {
  (void)dt;
  OkCamera *cam = OkCore::getCamera();
  if (cam == NULL) {
    return;
  }
  const OkRotation &cr = cam->getRotation();
  setRotation(OkRotation(cr.getPitch(), cr.getYaw(), 0.0f));

  // Proximity fade (see setProximityFade): modulate the tint alpha by
  // the camera distance so a quad near the lens vanishes instead of
  // filling the screen.
  if (proximityFade > 0.0f) {
    OkPoint p  = getPosition();
    OkPoint c  = cam->getPosition();
    float   dx = p.x() - c.x();
    float   dy = p.y() - c.y();
    float   dz = p.z() - c.z();
    float   d  = std::sqrt(dx * dx + dy * dy + dz * dz);
    float   f  = (d - proximityFade) / proximityFade;  // 0 at near, 1 at 2x
    if (f < 0.0f) {
      f = 0.0f;
    }
    if (f > 1.0f) {
      f = 1.0f;
    }
    tintColor[3] = baseAlpha * f;
  }
}

/**
 * @brief Rotation that points a +Z-facing quad at `from` toward `to`.
 *
 * The local +Z axis of a rotated object is
 * (sin(yaw)cos(pitch), -sin(pitch), cos(yaw)cos(pitch))
 * (third column of OkRotation::_updateMatrix), so solving
 * +Z = normalize(to - from) gives pitch and yaw directly. Roll is 0.
 */
OkRotation OkBillboard::facingRotation(const OkPoint &from,
                                       const OkPoint &to) {
  float dx = to.x() - from.x();
  float dy = to.y() - from.y();
  float dz = to.z() - from.z();
  float dl = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (dl < 1e-6f) {
    return OkRotation();
  }
  float pitch = std::asin(-dy / dl);
  float yaw   = std::atan2(dx, dz);
  return OkRotation(pitch, yaw, 0.0f);
}
