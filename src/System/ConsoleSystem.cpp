#include <System/ConsoleSystem.h>
#include <Script/ScriptEngine.h>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
}

void ConsoleSystem::executeLua(const std::string& code) {
    lua_State* L = m_scriptEngine->getRawState();

    std::string evalCode = "return " + code;
    if (luaL_dostring(L, evalCode.c_str()) == LUA_OK) {
        if (lua_gettop(L) > 0) {
            const char* res = lua_tostring(L, -1);
            if (res) {
                m_logger.log(std::string("[Result] ") + res);
            }
            lua_pop(L, 1);
        }
        return;
    } else {
        lua_pop(L, 1);
    }

    if (luaL_dostring(L, code.c_str()) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        m_logger.error(std::string("[Lua Error] ") + (err ? err : "Unknown error"));
        lua_pop(L, 1);
    }
}
