#include <UI/UIManager.h>
#include <Event/Event.h>

UIManager::UIManager(Shader& uiShader,
                     EventBus& eventBus,
                     KeyboardSystem& keyboard,
                     ConsoleSystem& console,
                     LoggerSystem& logger,
                     ResourceManager& resources,
                     float screenWidth, float screenHeight)
: m_renderer(uiShader, screenWidth, screenHeight),
m_eventBus(eventBus),
m_resources(resources),
m_screenWidth(screenWidth),
m_screenHeight(screenHeight) {

    m_rootUI.setPosition(0.0f, 0.0f);
    m_rootUI.setSize(screenWidth, screenHeight);
    m_rootUI.setPassThrough(true);

    // 1. 掛載設定選單 (先於 Console，確保 Console 絕對置頂)
    float menuW = screenWidth * 0.75f;
    float menuH = screenHeight * 0.75f;
    m_settingsMenu = m_rootUI.emplaceChild<UISettingsMenu>(
        (screenWidth - menuW) * 0.5f,
                                                           (screenHeight - menuH) * 0.5f,
                                                           menuW, menuH,
                                                           m_resources, m_eventBus
    );

    // 2. 掛載 Console
    m_console = m_rootUI.emplaceChild<UIConsole>(
        0.0f, 0.0f, screenWidth, screenHeight,
        m_eventBus, console, logger, m_resources
    );
    m_console->setName("InGameConsole");
    m_console->setVisible(false);

    // 3. 註冊波浪鍵呼叫 Console
    keyboard.registerShortcut(
        GLFW_KEY_GRAVE_ACCENT, 0,
        [this]() { toggleConsole(); }
    );

    // 4. ★ 註冊 ESC 呼叫設定選單 (由 UIManager 統一管理層級優先權)
    keyboard.registerShortcut(
        GLFW_KEY_ESCAPE, 0,
        [this]() { toggleSettingsMenu(); }
    );

    bindEvents();
}

void UIManager::toggleSettingsMenu() {
    if (!m_settingsMenu) return;

    // ★ 防禦 1：如果 Console 正開著，ESC 的優先權是關閉 Console，不觸發設定選單
    if (m_console && m_console->isVisible()) {
        toggleConsole();
        return;
    }

    // ★ 防禦 2：正常切換設定選單開關
    bool next = !m_settingsMenu->isVisible();
    m_settingsMenu->setVisible(next);

    if (next) {
        m_rootUI.setFocusedElement(m_settingsMenu);
    } else {
        if (m_rootUI.getFocusedElement() == m_settingsMenu) {
            m_rootUI.setFocusedElement(nullptr);
        }
    }
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
    // ★ 關鍵修復：視窗縮放時，重新計算設定選單的置中座標與寬高（占 75%）
    if (m_settingsMenu) {
        float menuW = width * 1.0f;
        float menuH = height * 1.0f;
        m_settingsMenu->setPosition((width - menuW) * 0.5f, (height - menuH) * 0.5f);
        m_settingsMenu->setSize(menuW, menuH);
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
