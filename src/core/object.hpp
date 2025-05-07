#ifndef OK_OBJECT_HPP
#define OK_OBJECT_HPP

#include "../math/point.hpp"
#include "../math/rotation.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class OkObject {
protected:
  OkPoint    position;
  OkRotation rotation;
  OkPoint    scaling;

  // Hierarchy
  OkObject *_parent{nullptr};
  OkObject *_firstChild{nullptr};
  OkObject *_nextSibling{nullptr};

public:
  OkObject();
  virtual ~OkObject();

  // Position
  OkPoint getPosition() const;
  void    setPosition(float x, float y, float z);
  void    move(float dx, float dy, float dz);

  // Rotation
  OkRotation getRotation() const;
  void       setRotation(float x, float y, float z);
  void       rotate(float dx, float dy, float dz);

  // Scale
  OkPoint getScaling() const {
    return scaling;
  }
  void setScaling(float x, float y, float z) {
    scaling = OkPoint(x, y, z);
  }

  // Hierarchy
  void attachTo(OkObject *parent);
  void detachFromParent();
  void detachAllChildren();

  // Transform matrix
  glm::mat4 getTransformMatrix() const;

protected:
  virtual void updateTransform() = 0;
};

#endif
