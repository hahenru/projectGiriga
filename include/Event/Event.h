#ifndef GIRIGARA_EVENT_EVENT_H
#define GIRIGARA_EVENT_EVENT_H

#include <UI/UIContainer.h>

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <Core/Depersonalization.h>
extern "C" {
    #include <lua.h>
}

// 基礎事件類別
class Event {
public:
    virtual ~Event() = default;
    mutable bool isHandled = false; // 是否已被消耗（阻止繼續向下派發）
};

// 鍵盤基礎事件
class KeyEvent : public Event {
public:
    KeyEvent(int key, int scancode, int action, int mods)
    : key(key), scancode(scancode), action(action), mods(mods) {}

    int key;
    int scancode;
    int action;
    int mods;
};

// 鍵盤按下事件
class KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(int key, int scancode, int mods)
    : KeyEvent(key, scancode, 1 /* GLFW_PRESS */, mods) {}
};

// 鍵盤放開事件
class KeyReleasedEvent : public KeyEvent {
public:
    KeyReleasedEvent(int key, int scancode, int mods)
    : KeyEvent(key, scancode, 0 /* GLFW_RELEASE */, mods) {}
};

// 字元/文字輸入事件（供 UI 文字框、主控台輸入中英文字元）
class CharInputEvent : public Event {
public:
    explicit CharInputEvent(unsigned int codepoint)
    : codepoint(codepoint) {}

    unsigned int codepoint;
};

// 滑鼠移動事件
class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(double x, double y)
    : x(x), y(y) {}

    double x;
    double y;
};

// 滑鼠按鍵事件
class MouseButtonEvent : public Event {
public:
    MouseButtonEvent(int button, int action, int mods, double x, double y)
    : button(button), action(action), mods(mods), x(x), y(y) {}

    int button;
    int action;
    int mods;
    double x;
    double y;
};

// 視窗聚焦事件
class WindowFocusEvent : public Event {
public:
    explicit WindowFocusEvent(bool focused)
    : focused(focused) {}

    bool focused;
};

// 視窗緩衝區/大小改變事件
class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(int width, int height)
    : width(width), height(height) {}

    int width;
    int height;
};

// 當前場景 UI 樹變更事件（所有權直接移交）
class SceneUIChangedEvent : public Event {
public:
    explicit SceneUIChangedEvent(std::unique_ptr<UIContainer> newUI)
        : uiTree(std::move(newUI)) {}

    std::unique_ptr<UIContainer> uiTree;
};

class MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(double xoffset, double yoffset, int mods = 0)
    : xoffset(xoffset), yoffset(yoffset), mods(mods) {}

    double xoffset;
    double yoffset;
    int mods; // ★ 攜帶修飾鍵 (GLFW_MOD_CONTROL, GLFW_MOD_SHIFT 等)
};

// 1. 模組向 ScriptEngine 推送/撤銷專屬 Lua 函式
class RegisterScriptBindingEvent : public Event {
public:
    struct FuncEntry {
        std::string name;
        lua_CFunction func;
    };

    RegisterScriptBindingEvent(Depersonalization providerId,
                               std::string registryKey,
                               void* instancePtr,
                               std::vector<FuncEntry> functions)
    : providerId(providerId),
    registryKey(std::move(registryKey)),
    instancePtr(instancePtr),
    functions(std::move(functions)) {}

    Depersonalization providerId;
    std::string registryKey;
    void* instancePtr;
    std::vector<FuncEntry> functions;
};

class UnregisterScriptBindingEvent : public Event {
public:
    explicit UnregisterScriptBindingEvent(Depersonalization providerId)
    : providerId(providerId) {}

    Depersonalization providerId;
};

// 2. 遠端設定指令事件（Lua 腳本、Console 或 UI 皆可發布）
using RemoteSettingValue = std::variant<bool, int, float, std::string>;

class RemoteSettingRequestEvent : public Event {
public:
    // 依路徑修改 (如 "Audio.Volume.BGM")
    RemoteSettingRequestEvent(std::string path, RemoteSettingValue val, bool force = true)
    : path(std::move(path)), value(std::move(val)), force(force) {}

    // 依 UUID 修改
    RemoteSettingRequestEvent(uint64_t uuid, RemoteSettingValue val, bool force = true)
    : uuid(uuid), value(std::move(val)), force(force) {}

    std::string path;
    uint64_t uuid = 0;
    RemoteSettingValue value;
    bool force = true;
    bool succeeded = false; // 由 SettingsRegistry 處理後回填結果
};

// 3. 設定已實際變更廣播事件（通知 UISettingsMenu 同步刷新畫面控制項）
class SettingChangedEvent : public Event {
public:
    SettingChangedEvent(std::string path, uint64_t uuid, RemoteSettingValue val)
    : path(std::move(path)), uuid(uuid), value(std::move(val)) {}

    std::string path;
    uint64_t uuid;
    RemoteSettingValue value;
};

#endif // GIRIGARA_EVENT_EVENT_H
