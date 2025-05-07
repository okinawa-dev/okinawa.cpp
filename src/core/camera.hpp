#ifndef OK_CAMERA_HPP
#define OK_CAMERA_HPP

#include "../core/object.hpp"
#include "../math/point.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/**
 * @brief OkCamera class to handle camera properties and transformations.
 *        Inherits from OkObject to leverage position and rotation
 *        functionality.
 */
class OkCamera : public OkObject {
public:
  OkCamera(float width, float height);
  void setPerspective(float fovDegrees, float nearPlane, float farPlane);

  // Getters for matrices
  const glm::mat4 &getView() const { return view; }
  const glm::mat4 &getProjection() const { return projection; }
  const float     *getViewPtr() const { return glm::value_ptr(view); }
  const float *getProjectionPtr() const { return glm::value_ptr(projection); }

  // Camera-specific direction handling
  void           setDirection(float x, float y, float z);
  void           setDirection(const OkPoint &direction);
  const OkPoint &getFront() const { return front; }

protected:
  // Override OkObject's transform update
  void updateTransform() override;

private:
  glm::mat4 view;
  glm::mat4 projection;
  float     aspectRatio;
  float     fov;
  float     near;
  float     far;

  OkPoint front;
  OkPoint up;

  void updateView();
};

#endif
