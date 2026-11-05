#include <Script/ScriptBindings.h>
#include <Audio/AudioSystem.h>      // ★ 1. 補上完整定義，解決 incomplete type
#include <Script/LuaRef.h>
#include <UI/UIElement.h>
#include <UI/UIRegistry.h>
#include <UI/UIWindow.h>
#include <System/LoggerSystem.h>

#include <iostream>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
}

namespace {
    constexpr const char* LOGGER_REGISTRY_KEY = "GIRIGARA_LOGGER_INSTANCE";
    constexpr const char* AUDIO_REGISTRY_KEY  = "GIRIGARA_AUDIO_INSTANCE";

    LoggerSystem* getLoggerFromLua(lua_State* L) {
        lua_pushstring(L, LOGGER_REGISTRY_KEY);
        lua_gettable(L, LUA_REGISTRYINDEX);
        auto* logger = static_cast<LoggerSystem*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return logger;
    }

    // ★ 2. 放在前面定義，讓底下的 lua_Audio_* 都能正常調用
    AudioSystem* getAudioFromLua(lua_State* L) {
        lua_pushstring(L, AUDIO_REGISTRY_KEY);
        lua_gettable(L, LUA_REGISTRYINDEX);
        auto* audio = static_cast<AudioSystem*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return audio;
    }

    // 1. 綁定 Update 回調: BindUIUpdate(elementPtr, function(self, dt) ... end)
    int lua_BindUIUpdate(lua_State* L) {
        auto* element = static_cast<UIElement*>(lua_touserdata(L, 1));
        if (!element || !lua_isfunction(L, 2)) {
            return luaL_error(L, "Invalid arguments to BindUIUpdate: expected (element, function)");
        }

        lua_pushvalue(L, 2);
        element->setScriptUpdate(LuaRef::fromStack(L));
        return 0;
    }

    // 2. 設置元件位置: UI_SetPosition(elementPtr, x, y)
    int lua_UI_SetPosition(lua_State* L) {
        auto* element = static_cast<UIElement*>(lua_touserdata(L, 1));
        float x = static_cast<float>(luaL_checknumber(L, 2));
        float y = static_cast<float>(luaL_checknumber(L, 3));

        if (element) {
            element->setPosition(x, y);
        }
        return 0;
    }

    // 3. 輸出引擎日誌: EngineLog("訊息")
    int lua_EngineLog(lua_State* L) {
        const char* msg = luaL_checkstring(L, 1);
        if (msg) {
            LoggerSystem* logger = getLoggerFromLua(L);
            if (logger) {
                logger->log(std::string("[Lua] ") + msg);
            } else {
                std::cout << "[Lua] " << msg << std::endl;
            }
        }
        return 0;
    }

    // 4. 依名字尋找 UI: FindUI("GlobalConsole")
    int lua_FindUI(lua_State* L) {
        const char* name = luaL_checkstring(L, 1);
        UIElement* elem = UIRegistry::getInstance().findByName(name);
        if (elem) {
            lua_pushlightuserdata(L, elem);
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    // 5. 依 UUID 尋找 UI: GetUIByUUID(uuid)
    int lua_GetUIByUUID(lua_State* L) {
        uint64_t uuid = static_cast<uint64_t>(luaL_checkinteger(L, 1));
        UIElement* elem = UIRegistry::getInstance().findByUUID(uuid);
        if (elem) {
            lua_pushlightuserdata(L, elem);
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    // 6. 取得 UUID: UI_GetUUID(elementPtr)
    int lua_UI_GetUUID(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        if (elem) {
            lua_pushinteger(L, static_cast<lua_Integer>(elem->getUUID()));
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    // 7. 取得名稱: UI_GetName(elementPtr)
    int lua_UI_GetName(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        if (elem) {
            lua_pushstring(L, elem->getName().c_str());
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    // 8. 設置可見性: UI_SetVisible(elementPtr, visible)
    int lua_UI_SetVisible(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        bool visible = lua_toboolean(L, 2) != 0;
        if (elem) {
            elem->setVisible(visible);
        }
        return 0;
    }

    // 9. 取得 X 座標: local x = UI_GetX(elem)
    static int lua_UI_GetX(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        if (elem) {
            lua_pushnumber(L, static_cast<lua_Number>(elem->getX()));
        } else {
            lua_pushnumber(L, 0.0);
        }
        return 1;
    }

    // 10. 取得 Y 座標: local y = UI_GetY(elem)
    static int lua_UI_GetY(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        if (elem) {
            lua_pushnumber(L, static_cast<lua_Number>(elem->getY()));
        } else {
            lua_pushnumber(L, 0.0);
        }
        return 1;
    }

    // 11. 判斷是否正在拖曳視窗
    static int lua_UI_IsDragging(lua_State* L) {
        auto* win = dynamic_cast<UIWindow*>(static_cast<UIElement*>(lua_touserdata(L, 1)));
        if (win) {
            lua_pushboolean(L, win->isDragging() ? 1 : 0);
        } else {
            lua_pushboolean(L, 0);
        }
        return 1;
    }

    // 12. 在場景建立按鈕
    int lua_CreateButton(lua_State* L) {
        float x = static_cast<float>(luaL_checknumber(L, 1));
        float y = static_cast<float>(luaL_checknumber(L, 2));
        float w = static_cast<float>(luaL_checknumber(L, 3));
        float h = static_cast<float>(luaL_checknumber(L, 4));
        const char* text = luaL_optstring(L, 5, "");
        (void)x; (void)y; (void)w; (void)h; (void)text;
        return 1;
    }

    // 13. 切換場景
    int lua_SwitchScene(lua_State* L) {
        const char* script = luaL_checkstring(L, 1);
        (void)script;
        return 0;
    }

    int lua_UI_SetPositionPercent(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        float x = static_cast<float>(luaL_checknumber(L, 2));
        float y = static_cast<float>(luaL_checknumber(L, 3));
        if (elem) {
            elem->setPositionPercent(x, y);
        }
        return 0;
    }

    // 14. 播放 BGM: Audio_PlayBGM("assets/Sounds/theme.wav", true)
    int lua_Audio_PlayBGM(lua_State* L) {
        const char* path = luaL_checkstring(L, 1);
        bool loop = lua_isboolean(L, 2) ? (lua_toboolean(L, 2) != 0) : true;

        if (path) {
            AudioSystem* audio = getAudioFromLua(L);
            if (audio) {
                audio->playBGM(path, loop);
            }
        }
        return 0;
    }

    // 15. 停止 BGM: Audio_StopBGM()
    int lua_Audio_StopBGM(lua_State* L) {
        AudioSystem* audio = getAudioFromLua(L);
        if (audio) {
            audio->stopBGM();
        }
        return 0;
    }

    // 16. 播放一次性音效: Audio_PlaySFX("assets/Sounds/click.wav")
    int lua_Audio_PlaySFX(lua_State* L) {
        const char* path = luaL_checkstring(L, 1);
        if (path) {
            AudioSystem* audio = getAudioFromLua(L);
            if (audio) {
                audio->playSFX(path);
            }
        }
        return 0;
    }
}

namespace ScriptBindings {
    // ★ 3. 實作簽名對齊 Header：補上 AudioSystem& audio
    void registerAll(lua_State* L, LoggerSystem& logger, AudioSystem& audio) {
        if (!L) return;

        // 保存 Logger 與 Audio 指標至 Lua 註冊表
        lua_pushstring(L, LOGGER_REGISTRY_KEY);
        lua_pushlightuserdata(L, &logger);
        lua_settable(L, LUA_REGISTRYINDEX);

        lua_pushstring(L, AUDIO_REGISTRY_KEY);
        lua_pushlightuserdata(L, &audio);
        lua_settable(L, LUA_REGISTRYINDEX);

        // 核心更新與輸出 API
        lua_register(L, "BindUIUpdate", lua_BindUIUpdate);
        lua_register(L, "UI_SetPosition", lua_UI_SetPosition);
        lua_register(L, "EngineLog", lua_EngineLog);

        // 物件定位與屬性 API
        lua_register(L, "FindUI", lua_FindUI);
        lua_register(L, "GetUIByUUID", lua_GetUIByUUID);
        lua_register(L, "UI_GetUUID", lua_UI_GetUUID);
        lua_register(L, "UI_GetName", lua_UI_GetName);
        lua_register(L, "UI_SetVisible", lua_UI_SetVisible);
        lua_register(L, "UI_GetX", lua_UI_GetX);
        lua_register(L, "UI_GetY", lua_UI_GetY);
        lua_register(L, "UI_IsDragging", lua_UI_IsDragging);
        lua_register(L, "CreateButton", lua_CreateButton);
        lua_register(L, "SetPositionPercent", lua_UI_SetPositionPercent);

        // 控制場景相關 API
        lua_register(L, "SwitchScene", lua_SwitchScene);

        // 音訊控制 API
        lua_register(L, "Audio_PlayBGM", lua_Audio_PlayBGM);
        lua_register(L, "Audio_StopBGM", lua_Audio_StopBGM);
        lua_register(L, "Audio_PlaySFX", lua_Audio_PlaySFX);
    }
}