#ifndef GIRIGARA_EVENT_EVENT_H
#define GIRIGARA_EVENT_EVENT_H

#include <UI/UIContainer.h>
#include <memory>

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

// 滑鼠滾輪事件
class MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(double xoffset, double yoffset)
    : xoffset(xoffset), yoffset(yoffset) {}

    double xoffset;
    double yoffset;
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

#endif // GIRIGARA_EVENT_EVENT_H
