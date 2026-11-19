#include <Script/ScriptEngine.h>
#include <Event/Event.h>
#include <utility>

namespace {
    template <typename T>
    bool isEventMainThreadOnly(const T& e) {
        if constexpr (requires { e.mainThreadOnly; }) {
            return e.mainThreadOnly;
        } else {
            return false;
        }
    }
}

ScriptEngine::ScriptEngine(EventBus& eventBus, ScriptDomain domain)
    : m_eventBus(eventBus),
      m_domain(domain) {
    m_L = luaL_newstate();
    if (m_L) {
        luaL_openlibs(m_L);
        bindEvents();
    } else {
        std::cerr << "[ScriptEngine] Failed to initialize Lua state.\n";
    }
}

ScriptEngine::~ScriptEngine() {
    release();
}

std::unique_ptr<ScriptEngine> ScriptEngine::fork(ScriptDomain targetDomain) const {
    auto child = std::make_unique<ScriptEngine>(m_eventBus, targetDomain);

    // 鎖住母體快照，將已註冊的 C++ 綁定複製給子 VM（子 VM 內部會自動過濾 mainThreadOnly）
    std::lock_guard<std::recursive_mutex> lock(m_luaMutex);
    for (const auto& [providerId, record] : m_boundModules) {
        child->applyModuleBinding(providerId, record);
    }

    return child;
}

void ScriptEngine::applyModuleBinding(Depersonalization providerId, const BoundModuleRecord& record) {
    if (!m_L) return;
    if (record.mainThreadOnly && m_domain != ScriptDomain::MainThread) {
        return; // WorkerThread 子 VM 自動屏蔽操作 UI / 音訊的綁定
    }

    std::lock_guard<std::recursive_mutex> lock(m_luaMutex);

    if (!record.registryKey.empty() && record.instancePtr) {
        lua_pushstring(m_L, record.registryKey.c_str());
        lua_pushlightuserdata(m_L, record.instancePtr);
        lua_settable(m_L, LUA_REGISTRYINDEX);
    }

    for (const auto& fn : record.functions) {
        if (fn.func) {
            lua_register(m_L, fn.name.c_str(), fn.func);
        }
    }

    m_boundModules[providerId] = record;
}

void ScriptEngine::removeModuleBinding(Depersonalization providerId) {
    if (!m_L) return;

    std::lock_guard<std::recursive_mutex> lock(m_luaMutex);

    auto it = m_boundModules.find(providerId);
    if (it == m_boundModules.end()) return;

    for (const auto& fn : it->second.functions) {
        lua_pushnil(m_L);
        lua_setglobal(m_L, fn.name.c_str());
    }

    if (!it->second.registryKey.empty()) {
        lua_pushstring(m_L, it->second.registryKey.c_str());
        lua_pushnil(m_L);
        lua_settable(m_L, LUA_REGISTRYINDEX);
    }

    m_boundModules.erase(it);
}

void ScriptEngine::bindEvents() {
    m_eventBus.subscribe<RegisterScriptBindingEvent>([this](RegisterScriptBindingEvent& e) {
        BoundModuleRecord record;
        record.registryKey = e.registryKey;
        record.instancePtr = e.instancePtr;
        record.mainThreadOnly = isEventMainThreadOnly(e);
        record.functions.reserve(e.functions.size());
        for (const auto& fn : e.functions) {
            record.functions.push_back({ fn.name, fn.func });
        }

        applyModuleBinding(e.providerId, record);
    }, m_id);

    m_eventBus.subscribe<UnregisterScriptBindingEvent>([this](UnregisterScriptBindingEvent& e) {
        removeModuleBinding(e.providerId);
    }, m_id);
}

void ScriptEngine::release() {
    m_eventBus.unsubscribeByOwner(m_id);

    std::lock_guard<std::recursive_mutex> lock(m_luaMutex);
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
    std::lock_guard<std::recursive_mutex> lock(m_luaMutex);
    if (!m_L) return false;

    int status = luaL_dofile(m_L, filepath.c_str());
    if (status != LUA_OK) {
        reportErrors(status);
        return false;
    }
    return true;
}

bool ScriptEngine::executeString(const std::string& code) {
    std::lock_guard<std::recursive_mutex> lock(m_luaMutex);
    if (!m_L) return false;

    int status = luaL_dostring(m_L, code.c_str());
    if (status != LUA_OK) {
        reportErrors(status);
        return false;
    }
    return true;
}

bool ScriptEngine::callFunction(const std::string& funcName) {
    std::lock_guard<std::recursive_mutex> lock(m_luaMutex);
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
    std::lock_guard<std::recursive_mutex> lock(m_luaMutex);
    if (!m_L) return false;

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