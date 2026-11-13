#ifndef GIRIGARA_SCRIPT_SCRIPT_ENGINE_H
#define GIRIGARA_SCRIPT_SCRIPT_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

#include <Core/Depersonalization.h>
#include <Event/EventBus.h>
#include <Resource/ScriptResource.h>

extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

class ScriptEngine {
public:
    explicit ScriptEngine(EventBus& eventBus);
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;
    ScriptEngine(ScriptEngine&& other) noexcept = delete;
    ScriptEngine& operator=(ScriptEngine&& other) noexcept = delete;

    // 載入並執行 Lua 檔案
    bool executeFile(const std::string& filepath);

    // 執行一段 Lua 字串
    bool executeString(const std::string& code);

    // 呼叫無參數的 Lua 全域函式
    bool callFunction(const std::string& funcName);
    bool executeResource(const ScriptResource& script);
    lua_State* getRawState() const { return m_L; }

private:
    struct BoundModuleRecord {
        std::string registryKey;
        std::vector<std::string> funcNames;
    };

    EventBus& m_eventBus;
    Depersonalization m_id;
    lua_State* m_L = nullptr;

    std::unordered_map<Depersonalization, BoundModuleRecord> m_boundModules;

    void bindEvents();
    void release();
    void reportErrors(int status);
};

#endif // GIRIGARA_SCRIPT_SCRIPT_ENGINE_H
