#ifndef GIRIGARA_SCENE_LUA_SCENE_H
#define GIRIGARA_SCENE_LUA_SCENE_H

#include <Scene/Scene.h>
#include <Script/ScriptEngine.h>
#include <string>

class LuaScene : public Scene {
public:
    LuaScene(SceneManager& manager,
             EventBus& bus,
             ResourceManager& resources,
             LoggerSystem& logger,
             ScriptEngine& scriptEngine,
             std::string scriptResourceName)
    : Scene(manager, bus, resources, logger),
      m_scriptEngine(scriptEngine),
      m_scriptResourceName(std::move(scriptResourceName)) {}

    ~LuaScene() override = default;

    void onEnter() override;
    void onExit() override;
    void update(float dt) override;
    void renderWorld() override {}

    std::unique_ptr<UIContainer> createUI() override;

private:
    ScriptEngine& m_scriptEngine;
    std::string m_scriptResourceName;
    std::unique_ptr<UIContainer> m_luaUI;
};

#endif // GIRIGARA_SCENE_LUA_SCENE_H