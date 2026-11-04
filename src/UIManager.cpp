#include <UI/UIManager.h>
#include <Event/Event.h>

UIManager::UIManager(Shader& uiShader,
                     FontLoader& defaultFont,
                     EventBus& eventBus,
                     KeyboardSystem& keyboard,
                     ConsoleSystem& console,
                     LoggerSystem& logger,
                     ResourceManager& resources,
                     float screenWidth, float screenHeight)
    : m_renderer(uiShader, screenWidth, screenHeight),
      m_font(defaultFont),
      m_eventBus(eventBus),
      m_resources(resources),
      m_screenWidth(screenWidth),
      m_screenHeight(screenHeight) {

    // 1. 全域根畫布保持穿透模式
    m_rootUI.setPosition(0.0f, 0.0f);
    m_rootUI.setSize(screenWidth, screenHeight);
    m_rootUI.setPassThrough(true);

    // 2. ★ 掛載 Console：尺寸設為 100% 全螢幕 (寬度 screenWidth, 高度 screenHeight)
    m_console = m_rootUI.emplaceChild<UIConsole>(
        0.0f, 0.0f, screenWidth, screenHeight,
        m_font, m_eventBus, console, logger, m_resources
    );
    m_console->setName("InGameConsole");
    m_console->setVisible(false);

    // 3. 綁定熱鍵
    keyboard.registerShortcut(
        GLFW_KEY_GRAVE_ACCENT, 0,
        [this]() { toggleConsole(); }
    );

    // 4. 監聽所有事件
    bindEvents();
}

void UIManager::onWindowResize(float width, float height) {
    m_screenWidth = width;
    m_screenHeight = height;
    m_renderer.setScreenSize(width, height);
    m_rootUI.setSize(width, height);

    // ★ 關鍵：視窗縮放時，Console 同步更新為 100% 全螢幕高
    if (m_console) {
        m_console->setSize(width, height);
    }
    if (m_activeSceneUI) {
        m_activeSceneUI->setSize(width, height);
    }
}

void UIManager::bindEvents() {
    // A. 場景 UI 移交事件
    m_eventBus.subscribe<SceneUIChangedEvent>([this](SceneUIChangedEvent& e) {
        if (m_activeSceneUI) {
            m_dyingSceneUI.push_back({ std::move(m_activeSceneUI), 0.0f, 0.25f });
        }

        m_activeSceneUI = std::move(e.uiTree);
        if (m_activeSceneUI) {
            m_activeSceneUI->setPosition(0.0f, 0.0f);
            m_activeSceneUI->setSize(m_screenWidth, m_screenHeight);
            m_activeSceneUI->setPassThrough(true); // 場景根容器也是穿透的
        }
    });

    // B. 視窗縮放事件
    m_eventBus.subscribe<WindowResizeEvent>([this](WindowResizeEvent& e) {
        onWindowResize(static_cast<float>(e.width), static_cast<float>(e.height));
    });

    // C. ★ 關鍵修正：滑鼠移動事件派發 (用於按鈕 Hover 更新)
    m_eventBus.subscribe<MouseMovedEvent>([this](MouseMovedEvent& e) {
        // 先由頂層全域 UI 處理 (若 Console 打開且滑鼠在其上，會攔截)
        bool handled = m_rootUI.handleMouseMove(e.x, e.y);
        
        // 若全域 UI 沒攔截 (例如 Console 隱藏或滑鼠在 Console 外)，再交給場景 UI
        if (!handled && m_activeSceneUI && m_activeSceneUI->isVisible()) {
            m_activeSceneUI->handleMouseMove(e.x, e.y);
        }
    });

    // D. ★ 關鍵修正：滑鼠點擊事件派發 (用於按鈕 Click 與輸入框 Focus)
    m_eventBus.subscribePriority<MouseButtonEvent>([this](MouseButtonEvent& e) {
        // 1. 優先送給頂層全域樹 (Console)
        if (m_rootUI.handleMouseButton(e.button, e.action, e.mods, e.x, e.y)) {
            e.isHandled = true; // 被 Console 攔截，底層按鈕絕不觸發
            return;
        }

        // 2. 全域樹未攔截，送給活躍場景 UI 樹
        if (m_activeSceneUI && m_activeSceneUI->isVisible()) {
            if (m_activeSceneUI->handleMouseButton(e.button, e.action, e.mods, e.x, e.y)) {
                e.isHandled = true;
            }
        }
    });
}

void UIManager::toggleConsole() {
    if (!m_console) return;
    bool next = !m_console->isVisible();
    m_console->setVisible(next);

    if (next) {
        m_rootUI.setFocusedElement(m_console);
        if (auto* input = m_console->getInputField()) {
            input->setFocused(true);
        }
    } else {
        // ★ 關閉時徹底清除焦點，防止幽靈焦點殘留
        m_rootUI.setFocusedElement(nullptr);
        if (auto* input = m_console->getInputField()) {
            input->setFocused(false);
        }
    }
}


void UIManager::update(float dt) {
    m_rootUI.update(dt);

    if (m_activeSceneUI) {
        m_activeSceneUI->update(dt);
    }

    for (auto it = m_dyingSceneUI.begin(); it != m_dyingSceneUI.end();) {
        it->currentLifetime += dt;
        it->tree->update(dt);
        if (it->currentLifetime >= it->maxLifetime) {
            it = m_dyingSceneUI.erase(it);
        } else {
            ++it;
        }
    }
}

void UIManager::render() {
    m_renderer.begin();

    // 1. 先畫即將消亡的場景 UI
    for (auto& entry : m_dyingSceneUI) {
        entry.tree->render(m_renderer);
    }

    // 2. 再畫當前活躍場景 UI
    if (m_activeSceneUI && m_activeSceneUI->isVisible()) {
        m_activeSceneUI->render(m_renderer);
    }

    // 3. 最後畫全域層 (Console / Toast 置頂)
    m_rootUI.render(m_renderer);

    m_renderer.end();
}