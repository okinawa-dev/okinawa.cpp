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
  void insertScene(OkScene *scene, const std::string &name, int index);
  void setScene(int index);
  void advance();
  void goBack();

  // Getters
  OkScene           *getCurrentScene() const { return currentScene; }
  const std::string &getCurrentSceneName() const { return currentSceneName; }
  int                getCurrentSceneIndex() const { return currentSceneIndex; }
  int getSceneCount() const { return static_cast<int>(collection.size()); }

private:
  static constexpr int     MAX_SCENES = 32;
  std::vector<OkSceneInfo> collection;
  int                      currentSceneIndex;
  std::string              currentSceneName;
  OkScene                 *currentScene;
};

#endif
