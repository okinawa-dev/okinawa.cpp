#include "ground_controller.hpp"

#include "../../core/camera.hpp"
#include "../../core/core.hpp"
#include "../../core/object.hpp"
#include "../../math/math.hpp"

OkGroundController::OkGroundController(float moveSpeed) {
  _moveSpeed       = moveSpeed;
  _referenceCamera = nullptr;
  _useActiveCamera = false;
}

OkGroundMove OkGroundController::computeGroundMove(const OkInputState &input,
                                                   const OkRotation   &frame,
                                                   float               speed,
                                                   float dtSeconds) {
  const float eps = 1e-4f;

  // Frame forward/right projected onto the ground plane (drop Y) so movement
  // stays horizontal regardless of the frame's pitch.
  OkPoint forward = frame.getForwardVector();
  OkPoint right   = frame.getRightVector();
  OkPoint forwardGround(forward.x(), 0.0f, forward.z());
  OkPoint rightGround(right.x(), 0.0f, right.z());
  if (forwardGround.magnitude() > eps)
    forwardGround = forwardGround.normalize();
  if (rightGround.magnitude() > eps)
    rightGround = rightGround.normalize();

  OkPoint direction(0.0f, 0.0f, 0.0f);
  if (input.forward)
    direction += forwardGround;
  if (input.backward)
    direction -= forwardGround;
  if (input.strafeRight)
    direction += rightGround;
  if (input.strafeLeft)
    direction -= rightGround;

  OkGroundMove result;
  result.moved     = false;
  result.dx        = 0.0f;
  result.dz        = 0.0f;
  result.facingYaw = frame.getYaw();

  if (direction.magnitude() > eps) {
    direction    = direction.normalize();
    OkPoint step = direction * (speed * dtSeconds);
    result.dx    = step.x();
    result.dz    = step.z();
    result.moved = true;
    float pitch  = 0.0f;
    float yaw    = 0.0f;
    OkMath::directionVectorToAngles(direction, pitch, yaw);
    result.facingYaw = yaw;
  }

  return result;
}

// The frame delta arrives in milliseconds; speeds are per second.
static const float kMsPerSecond = 1000.0f;

void OkGroundController::update(float dt, const OkInputState &input,
                                OkObject &controlled) {
  OkRotation frame = controlled.getRotation();
  if (_useActiveCamera) {
    OkCamera *active = OkCore::getCamera();
    if (active != nullptr) {
      frame = active->getRotation();
    }
  } else if (_referenceCamera != nullptr) {
    frame = _referenceCamera->getRotation();
  }

  OkGroundMove move =
      computeGroundMove(input, frame, _moveSpeed, dt / kMsPerSecond);
  if (move.moved) {
    controlled.move(move.dx, 0.0f, move.dz);
    controlled.setRotation(0.0f, move.facingYaw, 0.0f);
  }

  // Vertical nudge (held keys): straight up/down at the walk speed. Altitude
  // fix-ups -- e.g. climbing back above the terrain after a bad teleport.
  if (input.moveUp != input.moveDown) {
    float dy = _moveSpeed * (dt / kMsPerSecond);
    controlled.move(0.0f, input.moveUp ? dy : -dy, 0.0f);
  }
}
