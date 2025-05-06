#ifndef OK_CAMERA_HPP
#define OK_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/**
 * @brief OkCamera class to handle camera properties and transformations.
 */
class OkCamera {
public:
  OkCamera(float width, float height);
  void setView(const glm::mat4 &newView);
  void setPerspective(float fovDegrees, float nearPlane, float farPlane);

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

private:
  glm::mat4 view;
  glm::mat4 projection;
  float     aspectRatio;
  float     fov;
  float     near;
  float     far;
};

#endif
