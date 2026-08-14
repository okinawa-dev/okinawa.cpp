#ifndef OK_INSPECTION_CAMERA_HPP
#define OK_INSPECTION_CAMERA_HPP

#include "../core/camera.hpp"
#include "../math/point.hpp"

/**
 * @brief The camera a tool looks at a scene with: the wheel comes closer,
 *        a drag slides sideways, another drag swings the view round a
 *        point, and every one of those is scaled by how far above the
 *        scene the camera is.
 *
 *        That scaling is the whole reason this exists. A gesture worth a
 *        fixed number of metres is unusable in an application that is
 *        used at two ranges: the wheel crawls when the whole scene is on
 *        screen and throws the camera through a wall when a doorway is.
 *        Scaled by height, the same gesture means "a bit closer" at
 *        every range, which is what a person expects from a viewer for
 *        solid models.
 *
 *        It holds the arithmetic and none of the policy. Which button
 *        does what, whether the pointer is captured, where a pivot comes
 *        from -- these are questions only the application can answer,
 *        and the two it is most likely to want are the two only it can
 *        compute: the point under the cursor, which needs to know what
 *        the scene contains, and the height its ground sits at, which is
 *        a fact about that scene and not about cameras.
 *
 *        Distinct from OkSpectatorCamera, which flies from the keyboard
 *        through a world it is inside. This one is outside, looking in.
 */
class OkInspectionCamera : public OkCamera {
public:
  // How close a zoom or an orbit may bring the camera to what it is
  // aimed at, in world units. It approaches and never arrives: without
  // this a fast wheel ends inside the surface it was aimed at, and what
  // was being looked at is behind the camera.
  static constexpr float MIN_REACH = 2.0f;

  // How flat an orbit may get, as a fraction of the distance to the
  // pivot. Below it the camera is level with what it circles and the
  // view matrix runs out of an up direction.
  static constexpr float MIN_HEIGHT_FRACTION = 0.05f;

  // Defaults, in world units per wheel notch and per dragged pixel, at a
  // scale of 1. Chosen so a notch is a comfortable step and a drag keeps
  // the scene under the pointer at the ranges a tool is used at.
  static constexpr float DEFAULT_ZOOM_PER_NOTCH = 0.08f;
  static constexpr float DEFAULT_PAN_PER_PIXEL  = 0.0018f;

  // The clamps on the height the gestures are scaled by. Without the
  // lower one every gesture stops the moment the camera reaches the
  // ground; without the upper one a camera pulled far out crosses the
  // scene in one notch.
  static constexpr float DEFAULT_MIN_SCALE = 5.0f;
  static constexpr float DEFAULT_MAX_SCALE = 900.0f;

  OkInspectionCamera(const std::string &name, int width, int height);

  // The height the scene's ground sits at, which gestures are measured
  // from. A fact about the scene, so the application sets it.
  void setGroundHeight(float y) {
    _groundHeight = y;
  }
  float getGroundHeight() const {
    return _groundHeight;
  }

  void setScaleRange(float minHeight, float maxHeight);
  void setZoomPerNotch(float metres) {
    _zoomPerNotch = metres;
  }
  void setPanPerPixel(float metres) {
    _panPerPixel = metres;
  }

  /** @brief What a gesture is worth right now, in world units. */
  float gestureScale() const;

  /**
   * @brief Move along the view direction, by wheel notches.
   *
   *        The fallback for when there is nothing under the cursor to
   *        aim at -- pointing at the sky still has to do something.
   */
  void zoomAlongView(float notches);

  /**
   * @brief Move towards a point, by wheel notches, stopping short of it.
   *
   *        Towards what the cursor is over rather than along the middle
   *        of the view: the thing being pointed at then stays under the
   *        pointer while the camera closes in on it. Finding that point
   *        is the application's job, since only it knows what the scene
   *        is made of.
   */
  void zoomToward(const OkPoint &target, float notches);

  /**
   * @brief Slide across the view, by dragged pixels.
   *
   *        The signs are "take hold of the scene and move it": dragging
   *        right moves the camera left, so what was under the pointer
   *        stays under it. Reversed, it reads as the whole scene running
   *        backwards.
   */
  void panAcrossView(float dxPixels, float dyPixels);

  /**
   * @brief Swing the camera around a point of the scene.
   *
   *        Around the point, not around the camera's own position: what
   *        is being examined stays where it is while the view moves
   *        about it. Orbiting about the camera slides the subject out of
   *        frame, which is what a first-person game does and is wrong
   *        here.
   *
   * @return false when the move was refused for coming too close to the
   *         pivot or too near level with it, in which case nothing moved.
   */
  bool orbitAbout(const OkPoint &pivot, float yawDeg, float pitchDeg);

  // How high above the scene's ground the camera sits. Reported so the
  // rest of the engine can scale with it -- a pan controller multiplies
  // its pixels by this, and a camera that reported 0 would multiply a
  // tool's gestures away entirely.
  float viewDistance() const override;
  void  setViewDistance(float d) override;

private:
  float _groundHeight;
  float _minScale;
  float _maxScale;
  float _zoomPerNotch;
  float _panPerPixel;
};

#endif  // OK_INSPECTION_CAMERA_HPP
