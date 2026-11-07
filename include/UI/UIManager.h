#ifndef GIRIGARA_UI_UI_MANAGER_H
#define GIRIGARA_UI_UI_MANAGER_H

#include <UI/UIContainer.h>
#include <UI/UIConsole.h>
#include <UI/UISettingsMenu.h>
#include <Renderer/UIRenderer.h>
#include <Resource/ResourceManager.h>
#include <Font/FontLoader.h>
#include <Event/EventBus.h>
#include <System/KeyboardSystem.h>
#include <System/ConsoleSystem.h>
#include <System/LoggerSystem.h>
#include <memory>
#include <vector>

struct DyingUIEntry {
    std::unique_ptr<UIContainer> tree;
    float currentLifetime = 0.0f;
    float maxLifetime = 0.25f;
};

class UIManager {
public:
	UIManager(Shader& uiShader,
		EventBus& eventBus,
		KeyboardSystem& keyboard,
		ConsoleSystem& console,
		LoggerSystem& logger,
		ResourceManager& resources,
		float screenWidth, float screenHeight);

    ~UIManager() = default;

    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    void toggleConsole();
    void toggleSettingsMenu();
    UIConsole* getConsole() { return m_console; }
    UIContainer& getRootUI() { return m_rootUI; }

    void onWindowResize(float width, float height);
    void update(float dt);
    void render();

private:
    void bindEvents();

    UIRenderer m_renderer;
    EventBus& m_eventBus;
    ResourceManager& m_resources;

    // 三棵樹
    UIContainer m_rootUI;                                   // 1) 全局持久層 (Console, Toast 等)
    std::unique_ptr<UIContainer> m_activeSceneUI{nullptr};  // 2) 當前場景 UI 樹 (由 Event 注入)
    std::vector<DyingUIEntry> m_dyingSceneUI;               // 3) 延遲消亡隊列

    UIConsole* m_console{nullptr};
    UISettingsMenu* m_settingsMenu{nullptr};
    float m_screenWidth = 800.0f;
    float m_screenHeight = 600.0f;
};

#endif // GIRIGARA_UI_UI_MANAGER_H
