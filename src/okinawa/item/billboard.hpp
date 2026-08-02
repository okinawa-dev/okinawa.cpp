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
 * +Z). Every frame stepSelf() re-orients the item to the view plane of
 * the active camera (it adopts the camera's pitch and yaw, roll stays
 * 0), so the quad faces the camera and its content stays aligned with
 * the screen at every angle -- including straight under a top-down
 * camera, where position-based facing degenerates. Assign a texture
 * with setTexture() or loadTextureFromFile() as with any OkItem;
 * without one the quad renders with the flat fill colour.
 *
 * A billboard is expected to live at scene root (not attached under a
 * transformed parent).
 */
class OkBillboard : public OkItem {
protected:
  void stepSelf(float dt) override;
  float proximityFade;
  float baseAlpha;     // tint alpha before the fade modulation
  float cameraOffset;  // depth bias toward the camera, metres
  float anchor[3];     // position as set by the caller (offset applied on top)
  float applied[3];    // position this class last wrote, to detect moves
  bool  anchorValid;

public:
  OkBillboard(const std::string &name, float width, float height);

  // Proximity fade: the quad's tint alpha falls to zero as the camera
  // approaches (fully faded at `nearDist`, full alpha beyond
  // `nearDist * 2`). 0 disables. Essential for light halos: a quad
  // crossing the camera plane would otherwise fill the screen.
  void setProximityFade(float nearDist) { proximityFade = nearDist; }

  // Camera offset (depth bias): each frame the quad is drawn `metres`
  // closer to the camera ALONG THE VIEW RAY. Since a point moved along
  // its own view ray projects to the same pixel, the quad does not
  // shift on screen at all -- it only wins the depth test against the
  // object it belongs to. The classic fix for a light corona being
  // sliced by its own lamp. 0 disables.
  //
  // setPosition() sets the ANCHOR: the offset is applied on top of it
  // every frame, never accumulated.
  void setCameraOffset(float metres) { cameraOffset = metres; }

  // The rotation that turns a +Z-facing quad placed at `from` toward
  // `to` (pitch and yaw; roll always 0). Static so the math is testable
  // without a GL context.
  static OkRotation facingRotation(const OkPoint &from, const OkPoint &to);
};

#endif
