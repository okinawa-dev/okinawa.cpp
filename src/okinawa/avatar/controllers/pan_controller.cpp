#include "pan_controller.hpp"

#include "../../core/camera.hpp"
#include "../../core/core.hpp"
#include "../../core/object.hpp"
#include "../../input/input.hpp"

OkPanController::OkPanController(float speedPerPixel) {
  _speedPerPixel = speedPerPixel;
}

void OkPanController::update(float dt, const OkInputState &input,
                             OkObject &controlled) {
  (void)dt;  // pan follows the mouse delta directly, not the frame time

  if (input.panX == 0.0f && input.panY == 0.0f) {
    return;
  }

  OkCamera *camera = OkCore::getCamera();
  if (camera == nullptr) {
    return;
  }
  float distance = camera->viewDistance();
  if (distance <= 0.0f) {
    distance = 10.0f;  // base/spectator cameras: a sane constant speed
  }

  // On-screen axes on the ground plane: screen right = the camera's right
  // vector; screen up = the camera's up vector projected on XZ (for a
  // straight-down camera the up vector IS horizontal). Both normalized, so
  // the object follows the mouse whatever the camera orientation.
  const float eps   = 1e-4f;
  OkRotation  rot   = camera->getRotation();
  OkPoint     right = rot.getRightVector();
  OkPoint     up    = rot.getUpVector();
  OkPoint     rightGround(right.x(), 0.0f, right.z());
  OkPoint     upGround(up.x(), 0.0f, up.z());
  if (rightGround.magnitude() > eps) {
    rightGround = rightGround.normalize();
  }
  if (upGround.magnitude() > eps) {
    upGround = upGround.normalize();
  } else {
    // Camera up is vertical (looking horizontally): fall back to its ground
    // forward so "mouse up" still means "away from the camera".
    OkPoint forward = rot.getForwardVector();
    upGround        = OkPoint(forward.x(), 0.0f, forward.z());
    if (upGround.magnitude() > eps) {
      upGround = upGround.normalize();
    }
  }

  float   scale = _speedPerPixel * distance;
  OkPoint step  = rightGround * (input.panX * scale) +
                 upGround * (input.panY * scale);
  controlled.move(step.x(), 0.0f, step.z());
}
