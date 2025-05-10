#ifndef OK_SCENES_HPP
#define OK_SCENES_HPP

#include "../scene/scene.hpp"
#include <string>
#include <vector>

/**
 * @brief Structure to hold scene information.
 *        It contains a pointer to the scene and its name.
 */
struct OkSceneInfo {
  OkScene    *scene;
  std::string name;
};

/**
 * @brief Class to manage multiple scenes in the application.
 *        It allows adding, inserting, and switching between scenes.
 */
class OkSceneHandler {
public:
  // Constructor
  OkSceneHandler();

  // Scene management
  void addScene(OkScene *scene, const std::string &name);
  void insertScene(OkScene *scene, const std::string &name, size_t index);
  void setScene(size_t index);
  void advance();
  void goBack();

  // Getters
  OkScene           *getCurrentScene() const { return currentScene; }
  const std::string &getCurrentSceneName() const { return currentSceneName; }
  size_t             getCurrentSceneIndex() const { return currentSceneIndex; }
  size_t             getSceneCount() const { return collection.size(); }

private:
  static constexpr size_t  MAX_SCENES = 32;
  std::vector<OkSceneInfo> collection;
  size_t                   currentSceneIndex;
  std::string              currentSceneName;
  OkScene                 *currentScene;
};

#endif
