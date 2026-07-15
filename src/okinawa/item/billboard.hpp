#ifndef OK_BILLBOARD_HPP
#define OK_BILLBOARD_HPP

#include "../math/point.hpp"
#include "../math/rotation.hpp"
#include "item.hpp"
#include <string>

/**
 * A billboard: a rectangular quad that always renders facing the active
 * camera.
 *
 * The quad is built centred on the item's position, lying on the local
 * XY plane (width along X, height along Y, visible face toward local
 * +Z). Every frame stepSelf() re-orients the item so that its +Z axis
 * points at the active camera (spherical billboard: both yaw and pitch
 * follow; roll stays 0). Assign a texture with setTexture() or
 * loadTextureFromFile() as with any OkItem; without one the quad
 * renders with the flat fill colour.
 *
 * The facing uses the item's own position, so a billboard is expected
 * to live at scene root (not attached under a transformed parent).
 */
class OkBillboard : public OkItem {
protected:
  void stepSelf(float dt) override;

public:
  OkBillboard(const std::string &name, float width, float height);

  // The rotation that turns a +Z-facing quad placed at `from` toward
  // `to` (pitch and yaw; roll always 0). Static so the math is testable
  // without a GL context.
  static OkRotation facingRotation(const OkPoint &from, const OkPoint &to);
};

#endif
