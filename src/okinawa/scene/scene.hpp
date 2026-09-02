#ifndef OK_SCENE_HPP
#define OK_SCENE_HPP

#include "../core/object.hpp"
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class OkItem;

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

  // Remove an object from the scene and DELETE it. The scene owns what
  // it holds, so this is how a world that streams pieces in and out
  // gives memory back. Safe to call with an object the scene does not
  // hold (it is ignored). Invalidates the draw order.
  void removeObject(OkObject *object);
  void step(float dt);
  void draw();
  void activate();
  void deactivate();

  // Hierarchy lookup so a caller can address items by name (what the caller
  // then does with them -- visibility, colour, ... -- is the caller's concern).
  // findItem: first root OkItem with this exact name (or null).
  // findItems: every root OkItem whose name starts with `prefix` (empty = all).
  OkItem               *findItem(const std::string &name) const;
  std::vector<OkItem *> findItems(const std::string &prefix) const;

  /**
   * @brief Draw (or stop drawing) the origin axes of everything here.
   *
   * Every object carries the switch already; what was missing was a way
   * to reach all of them at once. It is a debugging view of where
   * things ARE -- what a rotation will turn about, where a piece is
   * anchored -- and the answer is only useful for the whole scene at
   * once: one object's axes tell you nothing about whether it sits
   * where its neighbours do.
   *
   * @return how many objects were switched, children included.
   */
  size_t setOriginAxes(bool on);

  // Getters
  bool isActive() const {
    return _isActive;
  }
  bool isPlayable() const {
    return _isPlayable;
  }
  bool isCurrent() const {
    return _isCurrent;
  }
  const std::string &getName() const {
    return name;
  }
  size_t getObjectCount() const {
    return rootObjects.size();
  }

  // Force the draw order to be rebuilt on the next frame (after adding
  // or removing objects in bulk).
  void invalidateDrawOrder() {
    _drawOrder.clear();
  }

private:
  // Opaque objects sorted near-to-far, refreshed periodically (see the
  // note in draw()).
  std::vector<std::pair<float, OkObject *>> _drawOrder;
  unsigned int                              _sortTimer = 0;

  std::string             name;
  bool                    _isActive;
  bool                    _isPlayable;
  bool                    _isCurrent;
  std::vector<OkObject *> rootObjects;  // Only stores objects without parents
};

#endif
