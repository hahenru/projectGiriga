#include <Scene/SceneManager.h>
#include <Scene/LuaScene.h>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
}

namespace {
    constexpr const char* SCENE_MANAGER_REGISTRY_KEY = "GIRIGARA_SCENE_MANAGER_INSTANCE";

    SceneManager* getSceneManagerFromLua(lua_State* L) {
        lua_pushstring(L, SCENE_MANAGER_REGISTRY_KEY);
        lua_gettable(L, LUA_REGISTRYINDEX);
        auto* mgr = static_cast<SceneManager*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return mgr;
    }

    // SwitchScene("MainMenuScript")
    int lua_SwitchScene(lua_State* L) {
        const char* script = luaL_checkstring(L, 1);
        if (script && script[0] != '\0') {
            if (SceneManager* mgr = getSceneManagerFromLua(L)) {
                mgr->switchToLuaScene(script);
            }
        }
        return 0;
    }
}

SceneManager::SceneManager(CoreContext& ctx)
: m_ctx(ctx) {
    registerScriptBindings();
}

SceneManager::~SceneManager() {
    unregisterScriptBindings();
    if (m_currentScene) {
        m_currentScene->onExit();
        m_currentScene.reset();
    }
}

void SceneManager::registerScriptBindings() {
    if (!m_ctx.eventBus) return;
    m_ctx.eventBus->publish(RegisterScriptBindingEvent{
        getID(),
                            SCENE_MANAGER_REGISTRY_KEY,
                            this,
                            {
                                { "SwitchScene", lua_SwitchScene }
                            }
    });
}

void SceneManager::unregisterScriptBindings() {
    if (!m_ctx.eventBus) return;
    m_ctx.eventBus->publish(UnregisterScriptBindingEvent{
        getID()
    });
}

void SceneManager::switchToLuaScene(const std::string& scriptResourceName) {
    switchScene<LuaScene>(scriptResourceName);
}

void SceneManager::update(float dt) {
    applyPendingScene();
    if (m_currentScene) {
        m_currentScene->update(dt);
    }
}

void SceneManager::applyPendingScene() {
    if (m_pendingSceneFactory) {
        auto factory = std::move(m_pendingSceneFactory);
        m_pendingSceneFactory = nullptr;
        factory();
    }
}
