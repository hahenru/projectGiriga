#include <System/KeyboardSystem.h>

KeyboardSystem::KeyboardSystem(EventBus& bus) {
    bus.subscribe<KeyPressedEvent>([this](KeyPressedEvent& e) {
        this->onKeyPressed(e);
    });
}

void KeyboardSystem::registerShortcut(int key, int mods, std::function<void()> action) {
    m_shortcuts[{key, mods}] = std::move(action);
}

void KeyboardSystem::onKeyPressed(KeyPressedEvent& e) {
    auto it = m_shortcuts.find({e.key, e.mods});
    if (it != m_shortcuts.end()) {
        it->second();
        e.isHandled = true; // 快捷鍵觸發後消耗該事件
    }
}
