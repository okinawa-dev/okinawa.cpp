#ifndef OK_CAMERA_HPP
#define OK_CAMERA_HPP

#include "../core/object.hpp"
#include "../math/ray.hpp"
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
  ~OkCamera() override = default;
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
  virtual void zoom(float delta) {
    (void)delta;
  }

  // Orbit interface: a camera that orbits its target (third-person). The MCP
  // `view` tool drives it absolutely -- yaw/pitch/distance around the avatar --
  // so any viewpoint is set and reproduced with one call. Base camera is not an
  // orbit; subclasses override. pitch is the LOOK pitch in degrees (negative =
  // looking down; -90 ~ top-down).
  virtual bool isOrbit() const {
    return false;
  }
  virtual void setOrbit(float yawDeg, float pitchDeg, float distance) {
    (void)yawDeg;
    (void)pitchDeg;
    (void)distance;
  }
  virtual float orbitYawDeg() const {
    return 0.0f;
  }
  virtual float orbitPitchDeg() const {
    return 0.0f;
  }
  virtual float orbitDistance() const {
    return 0.0f;
  }

  // How far this camera sits from what it observes (orbit distance, overhead
  // height, ...). 0 when the notion does not apply (base/spectator). Lets
  // consumers scale interactions with the visible area -- e.g. the pan
  // controller moves the avatar faster the further the camera is.
  virtual float viewDistance() const {
    return 0.0f;
  }
  // Write counterpart of viewDistance(): drive the camera's distance/height
  // directly (MCP `view`). Base ignores it; subclasses apply and clamp.
  virtual void setViewDistance(float d) {
    (void)d;
  }

  // Projection planes (the post-process depth linearization needs them)
  float getNearPlane() const {
    return near;
  }
  float getFarPlane() const {
    return far;
  }

  // Getters for matrices
  const glm::mat4 &getView() const {
    return view;
  }
  const glm::mat4 &getProjection() const {
    return projection;
  }
  const float *getViewPtr() const {
    return glm::value_ptr(view);
  }
  const float *getProjectionPtr() const {
    return glm::value_ptr(projection);
  }

  // Screen and world, in both directions. The pair belongs together: a
  // projection and an unprojection that disagree about which way y counts
  // is the kind of mistake that looks like a picking bug for an
  // afternoon, and keeping them side by side is what makes a round trip
  // an obvious thing to test.
  //
  // Both take the size of the surface being drawn to rather than reading
  // the camera's own aspect ratio, so a camera rendering into an
  // offscreen target answers about that target's pixels.

  /**
   * @brief The ray through a point on the window.
   *
   *        Unprojects the point at the near plane and again at the far
   *        one and subtracts, which is right for a perspective camera and
   *        for an orthographic one alike -- an orthographic ray does not
   *        pass through the eye, so a version built from the camera's
   *        position would be wrong for exactly the overhead views that
   *        tend to want picking.
   *
   * @param x      Cursor x in pixels, 0 at the left.
   * @param y      Cursor y in pixels, 0 at the top, the way a window
   *               system reports it. The flip to OpenGL's convention
   *               happens here so no caller has to remember it.
   * @param width  Surface width in pixels.
   * @param height Surface height in pixels.
   * @return A ray with a unit direction, so distances along it are world
   *         units. Degenerate arguments give a ray pointing along -Z from
   *         the origin, which hits nothing in particular.
   */
  OkRay rayThroughPixel(double x, double y, int width, int height) const;

  /**
   * @brief Where a world point lands on the window, in the same pixels
   *        rayThroughPixel takes.
   *
   *        Wanted by anything that draws over a 3D object -- a label, a
   *        marker, a leader line -- not only by picking.
   *
   * @return false when the point is behind the camera, where there is no
   *         pixel to report. The outputs are untouched then.
   */
  bool pixelOfPoint(const OkPoint &world, int width, int height, double *outX,
                    double *outY) const;

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
