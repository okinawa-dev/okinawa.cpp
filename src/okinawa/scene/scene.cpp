#include "scene.hpp"
#include "../math/frustum.hpp"
#include <algorithm>
#include "../item/item.hpp"
#include "../utils/logger.hpp"
#include "core/object.hpp"
#include <cstddef>
#include <string>

/**
 * @brief Constructor for the OkScene class.
 * @param name The name of the scene.
 */
OkScene::OkScene(const std::string &name) {
  this->name  = name;
  _isActive   = false;
  _isPlayable = false;
  _isCurrent  = false;

  OkLogger::info("Scene", "Created scene: " + name);
}

/**
 * @brief Destructor for the OkScene class.
 * Cleans up all root objects and their children.
 */
OkScene::~OkScene() {
  // Clean up root objects - their children will be deleted recursively
  for (size_t i = 0; i < rootObjects.size(); ++i) {
    delete rootObjects[i];
  }

  rootObjects.clear();
}

/**
 * @brief Remove an object from the scene and delete it.
 *
 *        The scene owns its objects, so removing is also freeing:
 *        anything that streams pieces of a world in and out needs this
 *        to give the memory back. The draw order is invalidated because
 *        it holds pointers into what just went away.
 */
void OkScene::removeObject(OkObject *object) {
  if (object == nullptr) {
    return;
  }
  for (size_t i = 0; i < rootObjects.size(); ++i) {
    if (rootObjects[i] == object) {
      rootObjects.erase(rootObjects.begin() + (long)i);
      _drawOrder.clear();
      delete object;
      return;
    }
  }
}

/**
 * @brief Add an object to the scene.
 * @param object The object to add (can be OkItem, OkItemGroup, etc.).
 */
void OkScene::addObject(OkObject *object) {
  if (!object)
    return;

  // Only add objects that don't have a parent
  if (object->getParent() == nullptr) {
    rootObjects.push_back(object);
    _drawOrder.clear();
  } else {
    OkLogger::warning("Scene",
                      "Cannot add object with parent directly to scene");
  }
}

/**
 * @brief Find a root OkItem by exact name (first match), or null.
 */
OkItem *OkScene::findItem(const std::string &name) const {
  for (size_t i = 0; i < rootObjects.size(); ++i) {
    if (rootObjects[i]->getName() == name) {
      OkItem *item = dynamic_cast<OkItem *>(rootObjects[i]);
      if (item != nullptr) {
        return item;
      }
    }
  }
  return nullptr;
}

/**
 * @brief Every root OkItem whose name starts with `prefix` (empty matches all).
 */
std::vector<OkItem *> OkScene::findItems(const std::string &prefix) const {
  std::vector<OkItem *> out;
  for (size_t i = 0; i < rootObjects.size(); ++i) {
    OkItem *item = dynamic_cast<OkItem *>(rootObjects[i]);
    if (item == nullptr) {
      continue;
    }
    const std::string &nm = item->getName();
    if (nm.compare(0, prefix.size(), prefix) == 0) {
      out.push_back(item);
    }
  }
  return out;
}

/**
 * @brief Update the scene and all its objects.
 * @param dt The delta time since the last update.
 */
void OkScene::step(float dt) {
  if (!_isActive)
    return;

  // Update root objects (they will update their children)
  for (size_t i = 0; i < rootObjects.size(); ++i) {
    rootObjects[i]->step(dt);
  }
}

/**
 * @brief Draw the scene and all its objects.
 */
void OkScene::draw() {
  if (!_isActive)
    return;

  // TWO PASSES: opaque geometry first, blended/additive objects (light
  // halos and glows) afterwards. Blended objects do not write depth on
  // purpose, so any opaque surface drawn after them would pass the
  // depth test and paint over them -- which is exactly what happened
  // when blended objects are created before opaque ones that end up
  // behind them.
  // Opaque geometry goes NEAREST FIRST. The depth buffer then rejects
  // hidden fragments early, so a wall behind another wall costs almost
  // nothing to shade -- the cheapest defence against overdraw in a
  // scene full of occluders. The order is refreshed periodically
  // rather than every frame: it only has to be roughly right, and
  // sorting thousands of objects every frame would cost more than it
  // saves.
  _sortTimer++;
  if (_drawOrder.size() != rootObjects.size() || (_sortTimer % 12) == 0) {
    _drawOrder.clear();
    _drawOrder.reserve(rootObjects.size());
    for (size_t i = 0; i < rootObjects.size(); ++i) {
      OkObject *o = rootObjects[i];
      if (o->isBlended()) {
        continue;
      }
      OkPoint p  = o->getPosition();
      float   dx = p.x() - OkFrustum::getViewerX();
      float   dy = p.y() - OkFrustum::getViewerY();
      float   dz = p.z() - OkFrustum::getViewerZ();
      _drawOrder.push_back(
          std::make_pair(dx * dx + dy * dy + dz * dz, o));
    }
    std::sort(_drawOrder.begin(), _drawOrder.end(),
              [](const std::pair<float, OkObject *> &a,
                 const std::pair<float, OkObject *> &b) {
                return a.first < b.first;
              });
  }
  for (size_t i = 0; i < _drawOrder.size(); ++i) {
    _drawOrder[i].second->draw();
  }
  for (size_t i = 0; i < rootObjects.size(); ++i) {
    if (rootObjects[i]->isBlended()) {
      rootObjects[i]->draw();
    }
  }
}

/**
 * @brief Activate the scene.
 */
void OkScene::activate() {
  _isActive  = true;
  _isCurrent = true;
}

/**
 * @brief Deactivate the scene.
 */
void OkScene::deactivate() {
  _isActive  = false;
  _isCurrent = false;
}
