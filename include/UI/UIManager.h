#ifndef GIRIGARA_UI_UI_MANAGER_H
#define GIRIGARA_UI_UI_MANAGER_H

#include <Core/Identifiable.h>
#include <UI/UIContainer.h>
#include <UI/UIConsole.h>
#include <UI/UISettingsMenu.h>
#include <Renderer/UIRenderer.h>
#include <Resource/IResourceManager.h>
#include <Font/FontLoader.h>
#include <Event/EventBus.h>
#include <System/KeyboardSystem.h>
#include <System/ConsoleSystem.h>
#include <System/LoggerSystem.h>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

struct DyingUIEntry {
    std::unique_ptr<UIContainer> tree;
    float currentLifetime = 0.0f;
    float maxLifetime = 0.25f;
};

class UIManager : public Identifiable {
public:
    UIManager(Shader& uiShader,
              EventBus& eventBus,
              KeyboardSystem& keyboard,
              ConsoleSystem& console,
              LoggerSystem& logger,
              IResourceManager& resources,
              float screenWidth, float screenHeight);

    ~UIManager() override;

    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    void toggleConsole();
    void toggleSettingsMenu();
    UIConsole* getConsole() { return m_console; }
    UIContainer& getRootUI() { return m_rootUI; }

    // 樹狀節點查詢與動態建立（供 C++ 與 Lua 腳本共用）
    UIElement* findElementByName(const std::string& name);
    UIElement* findElementByID(uint64_t id);
    UIElement* createButton(float x, float y, float w, float h, const std::string& text);

    void onWindowResize(float width, float height);
    void update(float dt);
    void render();

private:
    void bindEvents();
    void registerScriptBindings();
    void unregisterScriptBindings();

    UIRenderer m_renderer;
    EventBus& m_eventBus;
    IResourceManager& m_resources;

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
