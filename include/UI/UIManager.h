#ifndef GIRIGARA_UI_UI_MANAGER_H
#define GIRIGARA_UI_UI_MANAGER_H

#include <Core/Identifiable.h>
#include <UI/UIContainer.h>
#include <UI/UIFactory.h>
#include <Renderer/UIRenderer.h>
#include <Resource/IResourceManager.h>
#include <Event/EventBus.h>
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
              IResourceManager& resources,
              float screenWidth, float screenHeight);

    ~UIManager() override;

    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    // 通用全域覆蓋層掛載介面（供 Engine 內建 Console、開發者視窗、設定選單使用）
    template <typename T, typename... Args>
    T* emplaceOverlay(Args&&... args) {
        return m_overlayUI.emplaceChild<T>(std::forward<Args>(args)...);
    }
    void addOverlay(std::unique_ptr<UIElement> overlay);
    void bringOverlayToFront(UIElement* overlay);

    // 預留給未來的右鍵選單 / Tooltip 等最頂層彈出物
    UIContainer& getPopupLayer() { return m_popupUI; }
    UIContainer& getRootUI() { return m_overlayUI; }
    UIFactory& getFactory() { return m_factory; }

    // 樹狀節點查詢與通用工廠建立
    UIElement* findElementByName(const std::string& name);
    UIElement* findElementByID(uint64_t id);
    UIElement* createElement(const std::string& typeName, const UIPropMap& props = {}, UIContainer* parent = nullptr);

    void onWindowResize(float width, float height);
    void update(float dt);
    void render();

    IResourceManager& getResources() { return m_resources; }

private:
    void bindEvents();
    void registerScriptBindings();
    void unregisterScriptBindings();

    UIRenderer m_renderer;
    EventBus& m_eventBus;
    IResourceManager& m_resources;
    UIFactory m_factory;

    // 嚴格分層的四棵樹（由底到頂）
    std::vector<DyingUIEntry> m_dyingSceneUI;               // 1) 延遲消亡場景 UI
    std::unique_ptr<UIContainer> m_activeSceneUI{nullptr};  // 2) 當前場景 UI（遊戲/Lua 腳本 UI）
    UIContainer m_overlayUI;                                // 3) 引擎/系統覆蓋層（Console、開發者視窗、設定選單）
    UIContainer m_popupUI;                                  // 4) 最頂層彈出層（右鍵選單等）

    float m_screenWidth = 800.0f;
    float m_screenHeight = 600.0f;
};

#endif // GIRIGARA_UI_UI_MANAGER_H