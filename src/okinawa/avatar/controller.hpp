#ifndef OK_AVATAR_CONTROLLER_HPP
#define OK_AVATAR_CONTROLLER_HPP

class OkObject;
class OkInputState;

/**
 * @brief Input scheme for an avatar. An OkAvatar owns one controller; it
 *        interprets the per-frame input state and moves/acts on the controlled
 *        object. Different avatars (on foot, vehicle) use different
 * controllers, which is how switching avatar swaps the controls.
 */
class OkAvatarController {
public:
  virtual ~OkAvatarController() = default;

  /**
   * @brief Update the avatar for this frame. The controller obtains its own
   *        reference frame (e.g. a camera it holds), so it does not receive the
   *        rendered camera: control is independent of what is on screen.
   * @param dt        Delta time in milliseconds (engine loop units).
   * @param input     The current frame's input state.
   * @param controlled The controlled object to move/orient.
   */
  virtual void update(float dt, const OkInputState &input,
                      OkObject &controlled) = 0;
};

#endif  // OK_AVATAR_CONTROLLER_HPP
