#include "scene.hpp"
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

  OkLogger::info("Scene :: Created scene: " + name);
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
    OkLogger::warning(
        "Scene :: Cannot add object with parent directly to scene");
  }
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

  // Draw root objects (they will draw their children)
  for (size_t i = 0; i < rootObjects.size(); ++i) {
    rootObjects[i]->draw();
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
