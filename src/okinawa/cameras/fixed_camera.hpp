#ifndef OK_FIXED_CAMERA_HPP
#define OK_FIXED_CAMERA_HPP

#include "../core/camera.hpp"
#include "../math/point.hpp"

class OkObject;

/**
 * @brief Static, "Resident Evil"-style camera: stays at a fixed world position
 *        and optionally re-aims at the target each frame (so the avatar stays
 * in view as it moves through the room). Ignores the mouse. Use several of
 *        these, switching between them per area; a controller referencing the
 *        active camera then makes control room-relative.
 */
class OkFixedCamera : public OkCamera {
public:
  OkFixedCamera(const std::string &name, int width, int height);

  // Fix the camera at a world position; if lookAtTarget, it re-aims at the
  // observed target every frame, otherwise it keeps its current orientation.
  void place(const OkPoint &position, bool lookAtTarget = true);

  void updateForTarget(const OkObject *target, float dt) override;
  void look(float yawDeg, float pitchDeg) override {
    (void)yawDeg;
    (void)pitchDeg;
  }

private:
  OkPoint _position;
  bool    _lookAtTarget;
};

#endif  // OK_FIXED_CAMERA_HPP
