#include <System/LoggerSystem.h>
#include <iostream>
#include <sstream>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
}

namespace {
    constexpr const char* LOGGER_REGISTRY_KEY = "GIRIGARA_LOGGER_INSTANCE";

    int lua_EngineLog(lua_State* L) {
        const char* msg = luaL_checkstring(L, 1);
        if (!msg) return 0;

        lua_pushstring(L, LOGGER_REGISTRY_KEY);
        lua_gettable(L, LUA_REGISTRYINDEX);
        auto* logger = static_cast<LoggerSystem*>(lua_touserdata(L, -1));
        lua_pop(L, 1);

        if (logger) {
            logger->log(std::string("[Lua] ") + msg);
        } else {
            std::cout << "[Lua] " << msg << std::endl;
        }
        return 0;
    }
}

LoggerSystem::LoggerSystem(EventBus& eventBus, std::size_t maxHistoryLines)
: m_eventBus(eventBus),
m_maxHistoryLines(maxHistoryLines) {
    registerScriptBindings();
}

LoggerSystem::~LoggerSystem() {
    unregisterScriptBindings();
}

void LoggerSystem::log(const std::string& message) {
    std::cout << message << std::endl;

    std::istringstream stream(message);
    std::string line;
    while (std::getline(stream, line)) {
        m_logs.push_back(line);
        if (m_logs.size() > m_maxHistoryLines) {
            m_logs.pop_front();
        }
    }
    m_dirty = true;
}

void LoggerSystem::info(const std::string& message) {
    log("[INFO] " + message);
}

void LoggerSystem::warn(const std::string& message) {
    log("[WARN] " + message);
}

void LoggerSystem::error(const std::string& message) {
    log("[ERROR] " + message);
}

void LoggerSystem::clear() {
    m_logs.clear();
    m_dirty = true;
}

void LoggerSystem::registerScriptBindings() {
    m_eventBus.publish(RegisterScriptBindingEvent{
        getID(),
                       LOGGER_REGISTRY_KEY,
                       this,
                       {
                           { "EngineLog", lua_EngineLog }
                       }
    });
}

void LoggerSystem::unregisterScriptBindings() {
    m_eventBus.publish(UnregisterScriptBindingEvent{
        getID()
    });
}
