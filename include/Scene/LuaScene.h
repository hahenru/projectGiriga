#ifndef GIRIGARA_SCENE_LUASCENE_H
#define GIRIGARA_SCENE_LUASCENE_H

#include <Scene/Scene.h>
#include <Scene/SceneManager.h>
#include <Core/CoreContext.h>
#include <string>

class LuaScene : public Scene {
public:
    LuaScene(CoreContext& ctx, SceneManager& manager, std::string scriptResourceName)
        : Scene(ctx, manager), m_scriptResourceName(std::move(scriptResourceName)) {}

    void onEnter() override;
    void onExit() override;
    void update(float dt) override;
#ifndef GIRIGARA_SERVER
    std::unique_ptr<UIContainer> createUI() override;
#endif

private:
    std::string m_scriptResourceName;
};

#endif // GIRIGARA_SCENE_LUASCENE_H