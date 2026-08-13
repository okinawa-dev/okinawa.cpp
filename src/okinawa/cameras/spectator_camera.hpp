#ifndef OK_SPECTATOR_CAMERA_HPP
#define OK_SPECTATOR_CAMERA_HPP

#include "../core/camera.hpp"

class OkObject;

/**
 * @brief Free-fly spectator camera: ignores any target and flies from the input
 *        state (move relative to its own facing, full 3D), while the mouse
 *        (look) rotates it (the base free-fly behaviour). Drive it as the
 * active camera with no active avatar, e.g. a debug fly-through.
 */
class OkSpectatorCamera : public OkCamera {
public:
  // Free-flight speed, in metres per second.
  static constexpr float DEFAULT_MOVE_SPEED = 20.0f;

  OkSpectatorCamera(const std::string &name, int width, int height,
                    float moveSpeed = DEFAULT_MOVE_SPEED);

  void updateForTarget(const OkObject *target, float dt) override;

  void setMoveSpeed(float moveSpeed) {
    _moveSpeed = moveSpeed;
  }

private:
  float _moveSpeed;  // units per second
};

#endif  // OK_SPECTATOR_CAMERA_HPP
