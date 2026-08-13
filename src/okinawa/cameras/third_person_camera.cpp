#include "third_person_camera.hpp"

#include "../core/object.hpp"
#include "../math/math.hpp"

#include <algorithm>
#include <cmath>
#include <glm/trigonometric.hpp>

// pitch is the LOOK pitch: negative looks down (-89 ~ top-down), positive up.
static const float kMinPitch = glm::radians(-89.0f);
static const float kMaxPitch = glm::radians(30.0f);

OkThirdPersonCamera::OkThirdPersonCamera(const std::string &name, int width,
                                         int height, float distance,
                                         float focusHeight)
    : OkCamera(name, width, height) {
  _distance    = distance;
  _focusHeight = focusHeight;
  _yaw         = 0.0f;
  _pitch       = glm::radians(-20.0f);  // look slightly down at the avatar
}

void OkThirdPersonCamera::look(float yawDeg, float pitchDeg) {
  _yaw -= glm::radians(yawDeg);
  _pitch += glm::radians(pitchDeg);
  _pitch = std::max(kMinPitch, std::min(kMaxPitch, _pitch));
}

void OkThirdPersonCamera::zoom(float delta) {
  // Multiplicative so it feels even at any distance: each notch scales the
  // orbit radius by ~0.88, clamped to a sane gameplay range.
  _distance *= std::pow(0.88f, delta);
  _distance = std::max(2.0f, std::min(80.0f, _distance));
}

void OkThirdPersonCamera::setOrbit(float yawDeg, float pitchDeg,
                                   float distance) {
  // Absolute placement for the MCP `view` tool. Distance range is generous so a
  // top-down (pitch ~ -89, distance = height) can sit high above the avatar.
  _yaw      = glm::radians(yawDeg);
  _pitch    = std::max(kMinPitch, std::min(kMaxPitch, glm::radians(pitchDeg)));
  _distance = std::max(1.0f, std::min(2000.0f, distance));
}

float OkThirdPersonCamera::orbitYawDeg() const {
  return glm::degrees(_yaw);
}
float OkThirdPersonCamera::orbitPitchDeg() const {
  return glm::degrees(_pitch);
}

void OkThirdPersonCamera::updateForTarget(const OkObject *target, float dt) {
  (void)dt;
  if (target == nullptr) {
    return;
  }
  OkPoint focus = target->getPosition() + OkPoint(0.0f, _focusHeight, 0.0f);
  float   cp    = std::cos(_pitch);
  float   sp    = std::sin(_pitch);
  // Direction the camera LOOKS (toward the focus); the eye sits `distance` back
  // along the reverse, so the camera is behind + above and aims at the avatar.
  OkPoint look(std::sin(_yaw) * cp, sp, std::cos(_yaw) * cp);
  OkPoint eye = focus + look * (-_distance);
  setPosition(eye);
  setRotation(
      OkMath::lookAt(eye, focus));  // forward == look -> get_state matches
  setSpeed(0.0f, 0.0f, 0.0f);
}
