#include "spectator_camera.hpp"

#include "../core/core.hpp"
#include "../core/object.hpp"
#include "../input/input.hpp"

OkSpectatorCamera::OkSpectatorCamera(const std::string &name, int width,
                                     int height, float moveSpeed)
    : OkCamera(name, width, height) {
  _moveSpeed = moveSpeed;
}

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

  if (dir.magnitude() > 1e-4f) {
    dir          = dir.normalize();
    OkPoint step = dir * (_moveSpeed * (dt / 1000.0f));
    move(step.x(), step.y(), step.z());
  }
}
