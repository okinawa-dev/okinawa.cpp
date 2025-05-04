#include "scenes.hpp"
#include "../utils/logger.hpp"

/**
 * @brief Constructor for the OkSceneHandler class.
 *        Initializes the current scene index and reserves space for scenes.
 */
OkSceneHandler::OkSceneHandler() {
  currentSceneIndex = 0;
  currentScene      = nullptr;
  collection.reserve(MAX_SCENES);
}

/**
 * @brief Add a new scene to the collection.
 * @param scene The scene to add.
 * @param name  The name of the scene.
 */
void OkSceneHandler::addScene(OkScene *scene, const std::string &name) {
  if (collection.size() >= MAX_SCENES) {
    OkLogger::error("Scenes :: Cannot add more scenes, maximum reached");
    return;
  }

  OkLogger::info("Scenes :: Add Scene: " + name);

  collection.push_back({scene, name});
}

/**
 * @brief Insert a new scene at a specific index.
 * @param scene  The scene to insert.
 * @param name   The name of the scene.
 * @param index  The index at which to insert the scene.
 */
void OkSceneHandler::insertScene(OkScene *scene, const std::string &name,
                                 size_t index) {
  if (collection.size() >= MAX_SCENES) {
    OkLogger::error("Scenes :: Cannot add more scenes, maximum reached");
    return;
  }

  if (index > collection.size()) {
    OkLogger::error("Scenes :: Invalid index for scene insertion");
    return;
  }

  collection.insert(collection.begin() + index, {scene, name});
}

/**
 * @brief Set the current scene by index.
 * @param index The index of the scene to set as current.
 */
void OkSceneHandler::setScene(size_t index) {
  if (index >= collection.size()) {
    OkLogger::error("Scenes :: Invalid scene index");
    return;
  }

  // Deactivate current scene
  if (currentScene) {
    currentScene->deactivate();
  }

  // Update current scene info
  currentSceneIndex = index;
  currentSceneName  = collection[index].name;
  currentScene      = collection[index].scene;

  // Activate new scene
  currentScene->activate();

  OkLogger::info("Scenes :: Set Scene: " + currentSceneName + " (" +
                 std::to_string(index) + ")");
}

/**
 * @brief Advance to the next scene in the collection.
 */
void OkSceneHandler::advance() {
  if (currentSceneIndex + 1 >= collection.size()) {
    return;
  }
  setScene(currentSceneIndex + 1);
}

/**
 * @brief Go back to the previous scene in the collection.
 */
void OkSceneHandler::goBack() {
  if (currentSceneIndex == 0) {
    return;
  }
  setScene(currentSceneIndex - 1);
}
