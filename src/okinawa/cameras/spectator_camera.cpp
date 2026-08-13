#include "spectator_camera.hpp"

#include "../core/core.hpp"
#include "../core/object.hpp"
#include "../input/input.hpp"

OkSpectatorCamera::OkSpectatorCamera(const std::string &name, int width,
                                     int height, float moveSpeed)
    : OkCamera(name, width, height) {
  _moveSpeed = moveSpeed;
}

// Below this the movement input is nothing but float noise; the
// direction is not normalized and no step is taken.
static const float kMinInputMagnitude = 1e-4f;

// The frame delta arrives in milliseconds; the speed is per second.
static const float kMsPerSecond = 1000.0f;

void OkSpectatorCamera::updateForTarget(const OkObject *target, float dt) {
  (void)target;  // a spectator does not track anything
  OkInput *input = OkCore::getInput();
  if (input == nullptr) {
    return;
  }
  OkInputState state = input->getState();

  // Fly relative to where the camera looks (full 3D).
  OkPoint forward = getRotation().getForwardVector();
  OkPoint right   = getRotation().getRightVector();
  OkPoint dir(0.0f, 0.0f, 0.0f);
  if (state.forward)
    dir += forward;
  if (state.backward)
    dir -= forward;
  if (state.strafeRight)
    dir += right;
  if (state.strafeLeft)
    dir -= right;

  if (dir.magnitude() > kMinInputMagnitude) {
    dir          = dir.normalize();
    OkPoint step = dir * (_moveSpeed * (dt / kMsPerSecond));
    move(step.x(), step.y(), step.z());
  }
}
