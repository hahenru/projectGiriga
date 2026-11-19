#ifndef GIRIGARA_SCRIPT_SCRIPT_ENGINE_H
#define GIRIGARA_SCRIPT_SCRIPT_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <iostream>

#include <Core/Depersonalization.h>
#include <Event/EventBus.h>
#include <Resource/ScriptResource.h>

extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

enum class ScriptDomain {
    MainThread,     // Zygote 母體 / 主執行緒共用 VM（服務 UIElement、Console）
    WorkerThread    // 由 Zygote 孵化出的背景地圖專屬獨立 VM（零跨執行緒競爭）
};

class ScriptEngine {
public:
    explicit ScriptEngine(EventBus& eventBus, ScriptDomain domain = ScriptDomain::MainThread);
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;
    ScriptEngine(ScriptEngine&& other) noexcept = delete;
    ScriptEngine& operator=(ScriptEngine&& other) noexcept = delete;

    // ★ Zygote 孵化介面：複製當前母體的非主執行緒限定 C++ 綁定快照，建立全新的獨立 Lua VM
    std::unique_ptr<ScriptEngine> fork(ScriptDomain targetDomain = ScriptDomain::WorkerThread) const;

    // 執行緒安全地操作內部 lua_State*
    template <typename Func>
    auto withLock(Func&& fn) {
        std::lock_guard<std::recursive_mutex> lock(m_luaMutex);
        return fn(m_L);
    }

    bool executeFile(const std::string& filepath);
    bool executeString(const std::string& code);
    bool callFunction(const std::string& funcName);
    bool executeResource(const ScriptResource& script);

    ScriptDomain getDomain() const noexcept { return m_domain; }
    lua_State* getRawState() const noexcept { return m_L; }

private:
    struct CachedFuncEntry {
        std::string name;
        lua_CFunction func = nullptr;
    };

    struct BoundModuleRecord {
        std::string registryKey;
        void* instancePtr = nullptr;
        bool mainThreadOnly = false;
        std::vector<CachedFuncEntry> functions;
    };

    void applyModuleBinding(Depersonalization providerId, const BoundModuleRecord& record);
    void removeModuleBinding(Depersonalization providerId);

    void bindEvents();
    void release();
    void reportErrors(int status);

    EventBus& m_eventBus;
    ScriptDomain m_domain;
    Depersonalization m_id;
    lua_State* m_L = nullptr;
    mutable std::recursive_mutex m_luaMutex;

    std::unordered_map<Depersonalization, BoundModuleRecord> m_boundModules;
};

#endif // GIRIGARA_SCRIPT_SCRIPT_ENGINE_H