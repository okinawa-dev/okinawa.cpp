#ifndef OK_THIRD_PERSON_CAMERA_HPP
#define OK_THIRD_PERSON_CAMERA_HPP

#include "../core/camera.hpp"

class OkObject;

/**
 * @brief Camera that orbits behind/above the target and looks at it. The mouse
 *        (look) orbits it (yaw free, pitch clamped). Because it looks toward
 * the target, a camera-relative controller using this as its reference moves
 *        the avatar "into the screen".
 */
class OkThirdPersonCamera : public OkCamera {
public:
  OkThirdPersonCamera(const std::string &name, int width, int height,
                      float distance = 12.0f, float focusHeight = 1.5f);

  void updateForTarget(const OkObject *target, float dt) override;
  void look(float yawDeg, float pitchDeg) override;
  void zoom(float delta) override;

  void setDistance(float distance) { _distance = distance; }

  // Orbit interface (driven by the MCP `view` tool): absolute
  // yaw/pitch/distance.
  [[nodiscard]] bool  isOrbit() const override { return true; }
  void  setOrbit(float yawDeg, float pitchDeg, float distance) override;
  [[nodiscard]] float orbitYawDeg() const override;
  [[nodiscard]] float orbitPitchDeg() const override;
  [[nodiscard]] float orbitDistance() const override { return _distance; }
  [[nodiscard]] float viewDistance() const override { return _distance; }
  void  setViewDistance(float d) override {
    _distance = d < 1.0f ? 1.0f : (d > 2000.0f ? 2000.0f : d);
  }

private:
  float _distance;
  float _focusHeight;
  float _yaw;    // orbit angle around the target (radians)
  float _pitch;  // orbit elevation (radians, clamped)
};

#endif  // OK_THIRD_PERSON_CAMERA_HPP
