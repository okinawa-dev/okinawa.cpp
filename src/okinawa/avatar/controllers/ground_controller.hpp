#ifndef OK_GROUND_CONTROLLER_HPP
#define OK_GROUND_CONTROLLER_HPP

#include "../../input/input.hpp"
#include "../../math/rotation.hpp"
#include "../controller.hpp"

class OkCamera;
class OkObject;

/**
 * @brief Result of a ground-movement step: the XZ delta to apply and the yaw
 *        the controlled object should face. moved is false when there is no net
 *        input.
 */
struct OkGroundMove {
  bool  moved;
  float dx;
  float dz;
  float facingYaw;
};

/**
 * @brief Stock controller: movement on the ground plane (XZ) relative to a
 *        reference frame, turning the controlled object to face its movement.
 *        Works for a character, a vehicle, anything that moves on the ground.
 *
 *        The reference frame is decoupled from the rendered camera:
 *        - a reference camera (the usual gameplay camera) -> camera-relative,
 *        - the active rendered camera -> "Resident Evil" room-relative,
 *        - none -> relative to the controlled object's own facing.
 *        Y is left untouched (no terrain-follow yet).
 */
class OkGroundController : public OkAvatarController {
public:
  explicit OkGroundController(float moveSpeed = 5.0f);

  void update(float dt, const OkInputState &input,
              OkObject &controlled) override;

  void setMoveSpeed(float speed) {
    _moveSpeed = speed;
  }
  float getMoveSpeed() const {
    return _moveSpeed;
  }

  // Movement is relative to this camera (independent of what is rendered). Null
  // falls back to the controlled object's own facing.
  void setReferenceCamera(OkCamera *camera) {
    _referenceCamera = camera;
  }
  // Ignore the explicit reference and use the active rendered camera instead
  // (room-relative control, e.g. fixed-camera games).
  void setUseActiveCamera(bool useActive) {
    _useActiveCamera = useActive;
  }

  // Pure movement maths, unit-tested without a window/GL. Projects the frame's
  // forward/right onto the ground and returns the scaled XZ delta + facing yaw.
  static OkGroundMove computeGroundMove(const OkInputState &input,
                                        const OkRotation &frame, float speed,
                                        float dtSeconds);

private:
  float     _moveSpeed;        // units per second
  OkCamera *_referenceCamera;  // not owned
  bool      _useActiveCamera;
};

#endif  // OK_GROUND_CONTROLLER_HPP
