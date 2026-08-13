#ifndef OK_PAN_CONTROLLER_HPP
#define OK_PAN_CONTROLLER_HPP

#include "../controller.hpp"

class OkObject;
class OkInputState;

/**
 * @brief Mouse-pan controller: moves the controlled object on the ground plane
 *        (XZ) with the mouse alone -- no keys. Intended for overhead (top-down)
 *        views: the pan speed is proportional to the active camera's
 *        viewDistance(), so the further the camera, the faster the object
 *        crosses the map. The mouse wheel keeps zooming the camera as usual.
 *
 *        The pan delta comes from the per-frame input state (panX/panY, raw
 *        pixels, only fed while the cursor is captured). The pan direction is
 *        mapped through the active camera's on-screen axes (right vector and
 *        the up vector projected on the ground), so the object follows the
 *        mouse regardless of the camera's orientation. Y is left untouched.
 */
class OkPanController : public OkAvatarController {
public:
  // speedPerPixel = world units moved per mouse pixel, per unit of camera view
  // distance. 0.002 ~= a screen-height mouse sweep crosses the visible area.
  explicit OkPanController(float speedPerPixel = 0.002f);

  void update(float dt, const OkInputState &input,
              OkObject &controlled) override;

  void  setSpeedPerPixel(float speed) { _speedPerPixel = speed; }
  [[nodiscard]] float getSpeedPerPixel() const { return _speedPerPixel; }

private:
  float _speedPerPixel;
};

#endif  // OK_PAN_CONTROLLER_HPP
