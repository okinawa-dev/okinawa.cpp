#include "scene.hpp"
#include "../utils/logger.hpp"

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
 * Cleans up all root items and their children.
 */
OkScene::~OkScene() {
  // Clean up root items - their children will be deleted recursively
  for (size_t i = 0; i < rootItems.size(); ++i) {
    delete rootItems[i];
  }

  rootItems.clear();
}

/**
 * @brief Add an item to the scene.
 * @param item The item to add.
 */
void OkScene::addItem(OkItem *item) {
  if (!item)
    return;

  // Only add items that don't have a parent
  if (item->getParent() == nullptr) {
    rootItems.push_back(item);
  } else {
    OkLogger::warning("Scene :: Cannot add item with parent directly to scene");
  }
}

/**
 * @brief Update the scene and all its items.
 * @param dt The delta time since the last update.
 */
void OkScene::step(float dt) {
  if (!_isActive)
    return;

  // Update root items (they will update their children)
  for (size_t i = 0; i < rootItems.size(); ++i) {
    rootItems[i]->step(dt);
  }
}

/**
 * @brief Draw the scene and all its items.
 */
void OkScene::draw() {
  if (!_isActive)
    return;

  // Draw root items (they will draw their children)
  for (size_t i = 0; i < rootItems.size(); ++i) {
    rootItems[i]->draw();
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
