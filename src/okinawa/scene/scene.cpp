#include "scene.hpp"
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
 * @brief Add an object to the scene.
 * @param object The object to add (can be OkItem, OkItemGroup, etc.).
 */
void OkScene::addObject(OkObject *object) {
  if (!object)
    return;

  // Only add objects that don't have a parent
  if (object->getParent() == nullptr) {
    rootObjects.push_back(object);
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
  for (size_t i = 0; i < rootObjects.size(); ++i) {
    if (!rootObjects[i]->isBlended()) {
      rootObjects[i]->draw();
    }
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
