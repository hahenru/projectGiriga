#ifndef GIRIGARA_SCENE_MAIN_MENU_SCENE_H
#define GIRIGARA_SCENE_MAIN_MENU_SCENE_H

#include <Scene/Scene.h>
#include <UI/UIContainer.h>
#include <memory>

class MainMenuScene : public Scene {
public:
    using Scene::Scene;
    ~MainMenuScene() override = default;

    void onEnter() override;
    void onExit() override;
    void update(float dt) override { (void)dt; }
    void renderWorld() override {}

    // ★ 由此函式產出場景 UI 樹
    std::unique_ptr<UIContainer> createUI() override;
};

#endif // GIRIGARA_SCENE_MAIN_MENU_SCENE_H