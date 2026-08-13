#include "top_down_camera.hpp"

#include "../core/object.hpp"
#include "../math/math.hpp"

#include <algorithm>
#include <cmath>

OkTopDownCamera::OkTopDownCamera(const std::string &name, int width, int height,
                                 float height_m, float fovDegrees)
    : OkCamera(name, width, height) {
  _height = height_m;
  _fov    = fovDegrees;
}

// Each zoom notch lowers the overhead height by this much.
static const float kZoomPerNotch = 0.85f;

// The far plane has to clear the ground under the camera and then some,
// or the map fades out at the frame's edges where the ground is further
// away than straight down.
static const float kFarPerHeight = 2.0f;
static const float kFarSlack     = 2000.0f;

void OkTopDownCamera::zoom(float delta) {
  // Multiplicative so it feels even at any altitude: each notch in lowers the
  // overhead height by ~0.85, clamped to a sane range.
  _height *= std::pow(kZoomPerNotch, delta);
  const float minHeight = 10.0f;
  const float maxHeight = 1500.0f;
  _height               = std::max(minHeight, std::min(maxHeight, _height));
}

void OkTopDownCamera::updateForTarget(const OkObject *target, float dt) {
  (void)dt;
  if (target == nullptr) {
    return;
  }
  OkPoint p = target->getPosition();
  OkPoint eye(p.x(), p.y() + _height, p.z());
  OkPoint look(p.x(), p.y(), p.z());

  setPerspective(_fov, 1.0f, _height * kFarPerHeight + kFarSlack);
  setPosition(eye);
  // Straight down, world north (+Z) up on screen.
  setRotation(OkMath::lookAt(eye, look, OkPoint(0.0f, 0.0f, 1.0f)));
  setSpeed(0.0f, 0.0f, 0.0f);
}
