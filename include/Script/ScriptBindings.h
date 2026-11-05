#ifndef GIRIGARA_SCRIPT_SCRIPT_BINDINGS_H
#define GIRIGARA_SCRIPT_SCRIPT_BINDINGS_H

extern "C" {
    #include <lua.h>
}

class LoggerSystem;
class AudioSystem;

namespace ScriptBindings {
    // 註冊所有引擎與 UI 相關的 C API 至 Lua 虛擬機
    void registerAll(lua_State* L, LoggerSystem& logger, AudioSystem& audio);
}

#endif // GIRIGARA_SCRIPT_SCRIPT_BINDINGS_H
