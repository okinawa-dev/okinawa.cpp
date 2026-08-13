#ifndef OK_AVATAR_HPP
#define OK_AVATAR_HPP

#include "controller.hpp"

#include <vector>

class OkObject;
class OkCamera;
class OkInputState;

/**
 * @brief A controllable representation of the player in a scene: a controlled
 *        object plus the input scheme (controller) that drives it, plus a rig
 * of cameras that observe it. The avatar owns its controller (deleted in the
 *        destructor) but NOT the controlled object (the scene owns it) nor the
 *        rig cameras (OkCore owns them).
 *
 *        OkCore tracks the active avatar and updates it each frame; swapping
 * the active avatar (on foot -> car) changes controls and cameras in one step.
 * Control is independent of which camera is rendered: the controller carries
 * its own reference frame.
 */
class OkAvatar {
public:
  OkAvatar(OkObject *controlled, OkAvatarController *controller);
  ~OkAvatar();

  OkAvatar(const OkAvatar &)            = delete;
  OkAvatar &operator=(const OkAvatar &) = delete;

  // Move the controlled object from input, then reposition the rig cameras.
  void update(float dt, const OkInputState &input);

  OkObject *getControlledObject() const {
    return _controlled;
  }
  OkAvatarController *getController() const {
    return _controller;
  }
  void setController(OkAvatarController *controller);

  // Camera rig: cameras that observe this avatar, repositioned every frame so
  // they track it (and their gizmos show) even when not rendered. Not owned.
  void addCamera(OkCamera *camera);

private:
  OkObject               *_controlled;  // not owned (the scene owns it)
  OkAvatarController     *_controller;  // owned
  std::vector<OkCamera *> _cameras;     // rig, not owned (OkCore owns)
};

#endif  // OK_AVATAR_HPP
