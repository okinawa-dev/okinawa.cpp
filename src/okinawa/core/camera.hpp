#ifndef OK_CAMERA_HPP
#define OK_CAMERA_HPP

#include "../core/object.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

/**
 * @brief OkCamera class to handle camera properties and transformations.
 *        Inherits from OkObject to leverage position and rotation
 *        functionality.
 */
class OkCamera : public OkObject {
public:
  OkCamera(const std::string &name, int width, int height);
  virtual ~OkCamera() {}
  void setPerspective(float fovDegrees, float nearPlane, float farPlane);

  // Reposition this camera for the frame given the entity it observes (may be
  // null). Base camera does not track anything; subclasses (third-person,
  // top-down, fixed, ...) override.
  virtual void updateForTarget(const OkObject *target, float dt);
  // Apply a look delta in degrees (mouse / look equivalent). Base behaviour is
  // free-fly: rotate self with pitch clamped. Subclasses may orbit or ignore.
  virtual void look(float yawDeg, float pitchDeg);
  // Apply a zoom delta (mouse-wheel notches; + zooms in, - zooms out). Base
  // ignores it; subclasses pull the orbit closer (third-person distance) or
  // lower the overhead height (top-down). Repositioned by updateForTarget.
  virtual void zoom(float delta) { (void)delta; }

  // Orbit interface: a camera that orbits its target (third-person). The MCP
  // `view` tool drives it absolutely -- yaw/pitch/distance around the avatar --
  // so any viewpoint is set and reproduced with one call. Base camera is not an
  // orbit; subclasses override. pitch is the LOOK pitch in degrees (negative =
  // looking down; -90 ~ top-down).
  virtual bool  isOrbit() const { return false; }
  virtual void  setOrbit(float yawDeg, float pitchDeg, float distance) {
    (void)yawDeg;
    (void)pitchDeg;
    (void)distance;
  }
  virtual float orbitYawDeg() const { return 0.0f; }
  virtual float orbitPitchDeg() const { return 0.0f; }
  virtual float orbitDistance() const { return 0.0f; }

  // How far this camera sits from what it observes (orbit distance, overhead
  // height, ...). 0 when the notion does not apply (base/spectator). Lets
  // consumers scale interactions with the visible area -- e.g. the pan
  // controller moves the avatar faster the further the camera is.
  virtual float viewDistance() const { return 0.0f; }
  // Write counterpart of viewDistance(): drive the camera's distance/height
  // directly (MCP `view`). Base ignores it; subclasses apply and clamp.
  virtual void setViewDistance(float d) { (void)d; }

  // Getters for matrices
  const glm::mat4 &getView() const { return view; }
  const glm::mat4 &getProjection() const { return projection; }
  const float     *getViewPtr() const { return glm::value_ptr(view); }
  const float *getProjectionPtr() const { return glm::value_ptr(projection); }

  // Update and render
  void stepSelf(float dt) override;
  void drawSelf() override;

protected:
  // Override OkObject's transform update
  void updateTransformSelf() override;

private:
  glm::mat4 view;
  glm::mat4 projection;
  float     aspectRatio;
  float     fov;
  float     near;
  float     far;

  void updateView();
};

#endif
