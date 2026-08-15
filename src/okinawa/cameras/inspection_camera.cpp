#include "inspection_camera.hpp"

#include "math/rotation.hpp"
#include <algorithm>
#include <cmath>
#include <glm/trigonometric.hpp>

OkInspectionCamera::OkInspectionCamera(const std::string &name, int width,
                                       int height)
    : OkCamera(name, width, height) {
  _groundHeight = 0.0f;
  _minScale     = DEFAULT_MIN_SCALE;
  _maxScale     = DEFAULT_MAX_SCALE;
  _zoomPerNotch = DEFAULT_ZOOM_PER_NOTCH;
  _panPerPixel  = DEFAULT_PAN_PER_PIXEL;
}

void OkInspectionCamera::setScaleRange(float minHeight, float maxHeight) {
  _minScale = minHeight;
  _maxScale = std::max(maxHeight, minHeight);
}

float OkInspectionCamera::gestureScale() const {
  float above = getPosition().y() - _groundHeight;
  above       = std::max(above, _minScale);
  above       = std::min(above, _maxScale);
  return above;
}

void OkInspectionCamera::zoomAlongView(float notches) {
  OkPoint at      = getPosition();
  OkPoint forward = getRotation().getForwardVector();
  float   step    = notches * _zoomPerNotch * gestureScale();
  setPosition(OkPoint(at.x() + forward.x() * step, at.y() + forward.y() * step,
                      at.z() + forward.z() * step));
}

void OkInspectionCamera::zoomToward(const OkPoint &target, float notches) {
  OkPoint at    = getPosition();
  OkPoint away  = target - at;
  float   reach = away.magnitude();
  if (reach <= MIN_REACH) {
    return;  // already as close as it is allowed to get
  }

  float step = notches * _zoomPerNotch * gestureScale();
  // Never travel past the thing being approached. Pulling back has no
  // such limit: there is nothing behind the camera to overshoot.
  if (step > 0.0f) {
    step = std::min(step, reach - MIN_REACH);
  }
  OkPoint direction = away * (1.0f / reach);
  setPosition(OkPoint(at.x() + direction.x() * step,
                      at.y() + direction.y() * step,
                      at.z() + direction.z() * step));
}

void OkInspectionCamera::panAcrossView(float dxPixels, float dyPixels) {
  if (dxPixels == 0.0f && dyPixels == 0.0f) {
    return;
  }
  float   scale = gestureScale();
  OkPoint right = getRotation().getRightVector();
  OkPoint up    = getRotation().getUpVector();
  float   dx    = -dxPixels * _panPerPixel * scale;
  float   dy    = dyPixels * _panPerPixel * scale;
  OkPoint at    = getPosition();
  setPosition(OkPoint(at.x() + right.x() * dx + up.x() * dy,
                      at.y() + right.y() * dx + up.y() * dy,
                      at.z() + right.z() * dx + up.z() * dy));
}

bool OkInspectionCamera::orbitAbout(const OkPoint &pivot, float yawDeg,
                                    float pitchDeg) {
  OkPoint eye = getPosition();
  float   ox  = eye.x() - pivot.x();
  float   oy  = eye.y() - pivot.y();
  float   oz  = eye.z() - pivot.z();

  // Around the world's vertical axis first, then around the camera's own
  // right axis: the two rotations every orbit is made of.
  //
  // The sign has to match what look() does with the same angle, because
  // an orbit is a RIGID turn of the camera about the pivot: the position
  // goes round it and the facing turns by the same amount, so nothing in
  // the picture slides and the pivot keeps its pixel. Turned opposite
  // ways -- which is how this arithmetic arrived from the application it
  // was lifted from -- the two rotations partly cancel and the view
  // drifts off whatever was being examined. look() reads +yaw as looking
  // right, which is this direction.
  float ay = glm::radians(yawDeg);
  float cy = std::cos(ay);
  float sy = std::sin(ay);
  float rx = ox * cy - oz * sy;
  float rz = ox * sy + oz * cy;
  ox       = rx;
  oz       = rz;

  // The axis the tilt turns about is the camera's right -- as it will be
  // AFTER the yaw above, not as it was before. The facing is turned by
  // both angles at once, so an axis taken before the yaw belongs to a
  // camera that no longer exists, and a drag that mixes the two leaves
  // the pivot a dozen pixels from where it was grabbed.
  OkPoint aimed = getRotation().getRightVector();
  OkPoint right(aimed.x() * cy - aimed.z() * sy, aimed.y(),
                aimed.x() * sy + aimed.z() * cy);
  float   ap = glm::radians(pitchDeg);
  float   cp = std::cos(ap);
  float   sp = std::sin(ap);
  // Rodrigues about the right axis, which is horizontal by construction,
  // so the roll stays at zero and the horizon stays level.
  float dot = ox * right.x() + oy * right.y() + oz * right.z();
  float crx = right.y() * oz - right.z() * oy;
  float cry = right.z() * ox - right.x() * oz;
  float crz = right.x() * oy - right.y() * ox;
  float nx  = ox * cp + crx * sp + right.x() * dot * (1.0f - cp);
  float ny  = oy * cp + cry * sp + right.y() * dot * (1.0f - cp);
  float nz  = oz * cp + crz * sp + right.z() * dot * (1.0f - cp);

  // Never inside the pivot and never level with it: at either the view
  // matrix has no up direction left to work with.
  float reach = std::sqrt(nx * nx + ny * ny + nz * nz);
  if (reach < MIN_REACH || ny < reach * MIN_HEIGHT_FRACTION) {
    return false;
  }

  setPosition(OkPoint(pivot.x() + nx, pivot.y() + ny, pivot.z() + nz));
  look(yawDeg, pitchDeg);
  return true;
}

float OkInspectionCamera::viewDistance() const {
  return getPosition().y() - _groundHeight;
}

void OkInspectionCamera::setViewDistance(float d) {
  OkPoint at = getPosition();
  setPosition(OkPoint(at.x(), _groundHeight + d, at.z()));
}
