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

  // Direction setters/getters
  void setDirection(const glm::vec3 &direction) {
    front = glm::normalize(direction);
    updateView();
  }

  const glm::vec3 &getFront() const {
    return front;
  }
  const glm::vec3 &getPosition() const {
    return position;
  }
  void setPosition(const glm::vec3 &pos) {
    position = pos;
    updateView();
  }

  // Speed setters/getters
  void setSpeed(float newSpeed) {
    speed = newSpeed;
  }
  float getSpeed() const {
    return speed;
  }

private:
  glm::mat4 view;
  glm::mat4 projection;
  float     aspectRatio;
  float     fov;
  float     near;
  float     far;
  float     speed = 400.0f;

  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::vec3 front{0.0f, 0.0f, -1.0f};
  glm::vec3 up{0.0f, 1.0f, 0.0f};

  void updateView() {
    view = glm::lookAt(position, position + front, up);
  }
};

#endif
