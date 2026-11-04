#ifndef GIRIGARA_SCENE_GAME_SCENE_H
#define GIRIGARA_SCENE_GAME_SCENE_H

#include <Scene/Scene.h>
#include <UI/UIWindow.h>
#include <memory>

class GameScene : public Scene {
public:
    using Scene::Scene;
    ~GameScene() override = default;

    void onEnter() override;
    void onExit() override;
    void renderWorld() override {}

    std::unique_ptr<UIContainer> createUI() override;

private:
    UIWindow* m_consoleWindow = nullptr;
};

#endif // GIRIGARA_SCENE_GAME_SCENE_H