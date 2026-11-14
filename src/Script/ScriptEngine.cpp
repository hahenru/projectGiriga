#include <Script/ScriptEngine.h>
#include <Event/Event.h>
#include <utility>

ScriptEngine::ScriptEngine(EventBus& eventBus)
: m_eventBus(eventBus) {
    m_L = luaL_newstate();
    if (m_L) {
        luaL_openlibs(m_L); // 開啟標準函式庫
        bindEvents();
    } else {
        std::cerr << "[ScriptEngine] Failed to initialize Lua state.\n";
    }
}

ScriptEngine::~ScriptEngine() {
    release();
}

void ScriptEngine::bindEvents() {
    // 1. 接收各子系統推送過來的 Lua 函式綁定
    m_eventBus.subscribe<RegisterScriptBindingEvent>([this](RegisterScriptBindingEvent& e) {
        if (!m_L) return;

        // 存入子系統實體指標至 Lua Registry（供靜態 lua_CFunction 取回 this）
        if (!e.registryKey.empty() && e.instancePtr) {
            lua_pushstring(m_L, e.registryKey.c_str());
            lua_pushlightuserdata(m_L, e.instancePtr);
            lua_settable(m_L, LUA_REGISTRYINDEX);
        }

        // 註冊所有推送的 C 函式並記錄名稱，供日後解除綁定使用
        auto& record = m_boundModules[e.providerId];
        record.registryKey = e.registryKey;

        for (const auto& fn : e.functions) {
            lua_register(m_L, fn.name.c_str(), fn.func);
            record.funcNames.push_back(fn.name);
        }
    }, m_id);

    // 2. 接收子系統的解除綁定請求（模組卸載或切換場景時）
    m_eventBus.subscribe<UnregisterScriptBindingEvent>([this](UnregisterScriptBindingEvent& e) {
        if (!m_L) return;

        auto it = m_boundModules.find(e.providerId);
        if (it == m_boundModules.end()) return;

        // 將 Lua 全域環境中的對應函式設為 nil
        for (const auto& fnName : it->second.funcNames) {
            lua_pushnil(m_L);
            lua_setglobal(m_L, fnName.c_str());
        }

        // 清除 Lua Registry 中的指標
        if (!it->second.registryKey.empty()) {
            lua_pushstring(m_L, it->second.registryKey.c_str());
            lua_pushnil(m_L);
            lua_settable(m_L, LUA_REGISTRYINDEX);
        }

        m_boundModules.erase(it);
    }, m_id);
}

void ScriptEngine::release() {
    // 先向 EventBus 退訂自身監聽器，防止關閉後仍有事件打進來
    m_eventBus.unsubscribeByOwner(m_id);
    m_boundModules.clear();

    if (m_L) {
        lua_close(m_L);
        m_L = nullptr;
    }
}

void ScriptEngine::reportErrors(int status) {
    if (status != LUA_OK && m_L) {
        const char* msg = lua_tostring(m_L, -1);
        std::cerr << "[Lua Error] " << (msg ? msg : "Unknown error") << "\n";
        lua_pop(m_L, 1);
    }
}

bool ScriptEngine::executeFile(const std::string& filepath) {
    if (!m_L) return false;
    int status = luaL_dofile(m_L, filepath.c_str());
    if (status != LUA_OK) {
        reportErrors(status);
        return false;
    }
    return true;
}

bool ScriptEngine::executeString(const std::string& code) {
    if (!m_L) return false;
    int status = luaL_dostring(m_L, code.c_str());
    if (status != LUA_OK) {
        reportErrors(status);
        return false;
    }
    return true;
}

bool ScriptEngine::callFunction(const std::string& funcName) {
    if (!m_L) return false;
    lua_getglobal(m_L, funcName.c_str());
    if (!lua_isfunction(m_L, -1)) {
        lua_pop(m_L, 1);
        return false;
    }

    int status = lua_pcall(m_L, 0, 0, 0);
    if (status != LUA_OK) {
        reportErrors(status);
        return false;
    }
    return true;
}

bool ScriptEngine::executeResource(const ScriptResource& script) {
    int status = luaL_loadbuffer(m_L, script.getCode().c_str(), script.getCode().size(), script.getPath().c_str());
    if (status != LUA_OK) {
        reportErrors(status);
        return false;
    }

    status = lua_pcall(m_L, 0, 0, 0);
    if (status != LUA_OK) {
        reportErrors(status);
        return false;
    }
    return true;
}
