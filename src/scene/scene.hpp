#ifndef OK_SCENE_HPP
#define OK_SCENE_HPP

#include "../item/item.hpp"
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
  void addItem(OkItem *item);
  void step(float dt);
  void draw();
  void activate();
  void deactivate();

  // Getters
  bool               isActive() const { return _isActive; }
  bool               isPlayable() const { return _isPlayable; }
  bool               isCurrent() const { return _isCurrent; }
  const std::string &getName() const { return name; }
  size_t             getItemCount() const { return rootItems.size(); }

private:
  std::string           name;
  bool                  _isActive;
  bool                  _isPlayable;
  bool                  _isCurrent;
  std::vector<OkItem *> rootItems;  // Only stores items without parents
};

#endif
