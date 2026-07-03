#ifndef OK_TOP_DOWN_CAMERA_HPP
#define OK_TOP_DOWN_CAMERA_HPP

#include "../core/camera.hpp"

class OkObject;

/**
 * @brief Overhead camera: stays straight above the target, looking
 *        perpendicularly down with world north (+Z) up on screen, at a fixed
 *        height. Follows the target as it moves (debug / map view). Ignores the
 *        mouse.
 */
class OkTopDownCamera : public OkCamera {
public:
  OkTopDownCamera(const std::string &name, int width, int height,
                  float height_m = 400.0f, float fovDegrees = 60.0f);

  void updateForTarget(const OkObject *target, float dt) override;
  void look(float yawDeg, float pitchDeg) override {
    (void)yawDeg;
    (void)pitchDeg;
  }
  void zoom(float delta) override;

  void setHeight(float height_m) { _height = height_m; }

  float viewDistance() const override { return _height; }

private:
  float _height;
  float _fov;
};

#endif  // OK_TOP_DOWN_CAMERA_HPP
