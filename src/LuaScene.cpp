#include <Scene/LuaScene.h>

std::unique_ptr<UIContainer> LuaScene::createUI() {
    auto uiRoot = std::make_unique<UIContainer>();
    // 若 Lua 腳本有註冊 UI 元件，後續可在這裡組裝
    return uiRoot;
}

void LuaScene::onEnter() {
    try {
        const auto& script = m_resources.getScript(m_scriptResourceName);
        m_logger.log("[LuaScene] 載入腳本: " + script.getPath());

        if (m_scriptEngine.executeResource(script)) {
            m_scriptEngine.callFunction("OnSceneEnter");
        }
    } catch (const std::exception& e) {
        m_logger.error(std::string("[LuaScene 錯誤] ") + e.what());
    }
}

void LuaScene::onExit() {
    m_scriptEngine.callFunction("OnSceneExit");
    // UI 清理已由 UIManager 託管，不再手動清空 m_uiRoot
}

void LuaScene::update(float dt) {
    lua_State* L = m_scriptEngine.getRawState();
    if (L) {
        lua_getglobal(L, "OnSceneUpdate");
        if (lua_isfunction(L, -1)) {
            lua_pushnumber(L, static_cast<lua_Number>(dt));
            if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
                const char* err = lua_tostring(L, -1);
                m_logger.error(std::string("[LuaScene Update 錯誤] ") + (err ? err : "Unknown"));
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }
    }
}