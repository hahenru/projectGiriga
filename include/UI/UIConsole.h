#ifndef GIRIGARA_UI_UICONSOLE_H
#define GIRIGARA_UI_UICONSOLE_H

#include <UI/UIContainer.h>
#include <UI/UIScrollView.h>
#include <UI/UIInputField.h>
#include <UI/UIText.h>
#include <System/LoggerSystem.h>
#include <System/ConsoleSystem.h>
#include <Event/EventBus.h>
#include <Renderer/UIRenderer.h>
#include <Resource/IResourceManager.h>
#include <Shader/Shader.h>
#include <vector>
#include <cstddef>
#include <limits>

class UIConsole : public UIContainer {
public:
    UIConsole(float x, float y, float width, float height,
              EventBus& eventBus,
              ConsoleSystem& consoleSystem, LoggerSystem& logger,
              IResourceManager& resourceManager,
              Shader& bgShader);

    UIConsole(float x, float y, float width, float height,
              EventBus& eventBus,
              ConsoleSystem& consoleSystem, LoggerSystem& logger,
              IResourceManager& resourceManager);

    ~UIConsole() override = default;

    void setSize(float width, float height);
    void layoutChildren(float width, float height);

    void update(float dt) override;
    void render(UIRenderer& renderer) override;

    UIInputField* getInputField() { return m_inputField; }
	void setVisible(bool visible) override;

private:
    void initItemPool();
    void bindEvents();
    void syncVirtualViewport();

    static constexpr std::size_t kPoolCapacity = 150;
    static constexpr std::size_t kInvalidIndex = std::numeric_limits<std::size_t>::max();

    EventBus& m_eventBus;
    ConsoleSystem& m_consoleSystem;
    LoggerSystem& m_logger;
    IResourceManager& m_resources;
    Shader& m_bgShader;

    UIScrollView* m_scrollView{nullptr};
    UIInputField* m_inputField{nullptr};
    UIText* m_inputPrompt{nullptr};

    // 固定大小物件池：永遠只養 kPoolCapacity 個 UIText
    std::vector<UIText*> m_itemPool;
    std::vector<std::size_t> m_poolBoundIndex;
    std::size_t m_lastLogCount{0};
	
	void rebuildWrappedLines();
    void appendWrappedLog(const std::string& utf8Log);

    std::vector<std::u32string> m_wrappedLines; // 折行後的視覺行清單
    std::size_t m_syncedRawLogCount{0};

    float m_elapsedTime{0.0f};
    unsigned int m_fontSize{16};
};

#endif // GIRIGARA_UI_UICONSOLE_H