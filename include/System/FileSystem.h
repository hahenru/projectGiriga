#ifndef GIRIGARA_SYSTEM_FILE_SYSTEM_H
#define GIRIGARA_SYSTEM_FILE_SYSTEM_H

#include <Core/Identifiable.h>
#include <Event/EventBus.h>
#include <Script/LuaRef.h>
#include <System/LoggerSystem.h>
#include <string>
#include <vector>
#include <cstdint>

extern "C" {
    #include <lua.h>
}

class FileSystem : public Identifiable {
public:
    FileSystem(EventBus& eventBus, LoggerSystem& logger);
    ~FileSystem() override;

    FileSystem(const FileSystem&) = delete;
    FileSystem& operator=(const FileSystem&) = delete;

    // C++ 端檔案操作介面
    bool exists(const std::string& path) const;
    bool isDirectory(const std::string& path) const;
    bool isRegularFile(const std::string& path) const;
    bool readBinary(const std::string& path, std::vector<uint8_t>& outData) const;
    bool writeBinary(const std::string& path, const std::vector<uint8_t>& data) const;

private:
    void registerScriptBindings();
    void unregisterScriptBindings();
    void bindEvents();
    void handleFileDrop(const std::vector<std::string>& paths);

    // 註冊至 Lua 全域的 C 函式
    static int lua_RegisterFileDropCallback(lua_State* L);
    static int lua_FsStatPath(lua_State* L);
    static int lua_FsReadBinary(lua_State* L);
    static int lua_FsWriteBinary(lua_State* L);

    EventBus& m_eventBus;
    LoggerSystem& m_logger;
    LuaRef m_dropCallback;
};

#endif // GIRIGARA_SYSTEM_FILE_SYSTEM_H
