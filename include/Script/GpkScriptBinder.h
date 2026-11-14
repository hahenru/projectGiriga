#ifndef GIRIGARA_SCRIPT_GPK_SCRIPT_BINDER_H
#define GIRIGARA_SCRIPT_GPK_SCRIPT_BINDER_H

#include <Core/Depersonalization.h>
#include <Event/EventBus.h>
#include <Resource/GpkArchive.h>
#include <string>

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
    static int lua_GpkPackDirectory(lua_State* L);
    static int lua_GpkPackFiles(lua_State* L);
    static int lua_GpkList(lua_State* L);
    static int lua_GpkExtractAll(lua_State* L);
    static int lua_OpenFolderDialog(lua_State* L);
    static int lua_OpenFileDialog(lua_State* L);

    EventBus& m_eventBus;
    Depersonalization m_id;
};

#endif // GIRIGARA_SCRIPT_GPK_SCRIPT_BINDER_H
