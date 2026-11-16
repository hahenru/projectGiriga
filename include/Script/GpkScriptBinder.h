#ifndef GIRIGARA_SCRIPT_GPK_SCRIPT_BINDER_H
#define GIRIGARA_SCRIPT_GPK_SCRIPT_BINDER_H

#include <Core/Depersonalization.h>
#include <Event/EventBus.h>
#include <Resource/GpkArchive.h>
#include <Script/LuaRef.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

extern "C" {
    #include <lua.h>
}

class GpkScriptBinder {
public:
    explicit GpkScriptBinder(EventBus& eventBus);
    ~GpkScriptBinder();

    GpkScriptBinder(const GpkScriptBinder&) = delete;
    GpkScriptBinder& operator=(const GpkScriptBinder&) = delete;

    // 輔助工具：將 64 字元 Hex 字串或任意密碼短語轉換為 32-byte Key256
    static GpkArchive::Key256 parseKey(const std::string& keyStr);
    static GpkType parseType(const std::string& typeStr);
    static std::string typeToString(GpkType type);

private:
    struct WorkerSlot {
        std::shared_ptr<std::atomic<bool>> finished;
        std::jthread thread;
    };

    void onPackCompleted(GpkPackCompletedEvent& e);
    void cleanupFinishedWorkers();

    static int lua_GpkPackDirectory(lua_State* L);
    static int lua_GpkPackDirectoryAsync(lua_State* L);
    static int lua_GpkPackFiles(lua_State* L);
    static int lua_GpkList(lua_State* L);
    static int lua_GpkExtractAll(lua_State* L);
    static int lua_OpenFolderDialog(lua_State* L);
    static int lua_OpenFileDialog(lua_State* L);

    EventBus& m_eventBus;
    Depersonalization m_id;

    // 僅在主執行緒存取（派發 Lua 函式與註冊新任務皆在主執行緒）
    std::atomic<uint64_t> m_nextTaskId{ 1 };
    std::unordered_map<uint64_t, LuaRef> m_pendingCallbacks;

    std::mutex m_workersMutex;
    std::vector<WorkerSlot> m_workers;
};

#endif // GIRIGARA_SCRIPT_GPK_SCRIPT_BINDER_H