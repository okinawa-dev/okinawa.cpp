#ifndef OK_SCENE_HPP
#define OK_SCENE_HPP

#include "../core/object.hpp"
#include <cstddef>
#include <string>
#include <vector>

/**
 * @brief Class representing a scene in the application.
 *        It manages a collection of items and their hierarchy.
 */
class OkScene {
public:
  explicit OkScene(const std::string &name);
  ~OkScene();

  // Delete copy constructor and assignment
  OkScene(const OkScene &)            = delete;
  OkScene &operator=(const OkScene &) = delete;

  // Scene management
  void addObject(OkObject *object);
  void step(float dt);
  void draw();
  void activate();
  void deactivate();

  // Getters
  bool               isActive() const { return _isActive; }
  bool               isPlayable() const { return _isPlayable; }
  bool               isCurrent() const { return _isCurrent; }
  const std::string &getName() const { return name; }
  size_t             getObjectCount() const { return rootObjects.size(); }

private:
  std::string             name;
  bool                    _isActive;
  bool                    _isPlayable;
  bool                    _isCurrent;
  std::vector<OkObject *> rootObjects;  // Only stores objects without parents
};

#endif
