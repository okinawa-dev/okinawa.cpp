#ifndef OK_THIRD_PERSON_CAMERA_HPP
#define OK_THIRD_PERSON_CAMERA_HPP

#include "../core/camera.hpp"
#include <algorithm>

class OkObject;

/**
 * @brief Camera that orbits behind/above the target and looks at it. The mouse
 *        (look) orbits it (yaw free, pitch clamped). Because it looks toward
 * the target, a camera-relative controller using this as its reference moves
 *        the avatar "into the screen".
 */
class OkThirdPersonCamera : public OkCamera {
public:
  // Where the camera sits when nothing asks for anything else: far
  // enough back to see the avatar in its surroundings, focused at
  // roughly chest height so the head is not on the horizon.
  static constexpr float DEFAULT_DISTANCE     = 12.0f;
  static constexpr float DEFAULT_FOCUS_HEIGHT = 1.5f;

  // Orbit radius limits. The near one keeps the camera out of the
  // avatar; the far one is generous, because a top-down placement from
  // the MCP view tool is expressed as a very large distance.
  static constexpr float MIN_DISTANCE = 1.0f;
  static constexpr float MAX_DISTANCE = 2000.0f;

  OkThirdPersonCamera(const std::string &name, int width, int height,
                      float distance    = DEFAULT_DISTANCE,
                      float focusHeight = DEFAULT_FOCUS_HEIGHT);

  void updateForTarget(const OkObject *target, float dt) override;
  void look(float yawDeg, float pitchDeg) override;
  void zoom(float delta) override;

  void setDistance(float distance) {
    _distance = distance;
  }

  // Orbit interface (driven by the MCP `view` tool): absolute
  // yaw/pitch/distance.
  bool isOrbit() const override {
    return true;
  }
  void  setOrbit(float yawDeg, float pitchDeg, float distance) override;
  float orbitYawDeg() const override;
  float orbitPitchDeg() const override;
  float orbitDistance() const override {
    return _distance;
  }
  float viewDistance() const override {
    return _distance;
  }
  void setViewDistance(float d) override {
    _distance = std::min(std::max(d, MIN_DISTANCE), MAX_DISTANCE);
  }

private:
  float _distance;
  float _focusHeight;
  float _yaw;    // orbit angle around the target (radians)
  float _pitch;  // orbit elevation (radians, clamped)
};

#endif  // OK_THIRD_PERSON_CAMERA_HPP
