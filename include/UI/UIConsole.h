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

private:
    void bindEvents();
    void appendNewLogs();
    void updateFontSizeInPlace();

    EventBus& m_eventBus;
    ConsoleSystem& m_consoleSystem;
    LoggerSystem& m_logger;
    IResourceManager& m_resources;
    Shader& m_bgShader;

    UIScrollView* m_scrollView{nullptr};
    UIInputField* m_inputField{nullptr};
    UIText* m_inputPrompt{nullptr};

    // 直接快取每行 UIText 指標，改字體大小時原地修改，完全不重建物件
    std::vector<UIText*> m_logItems;
    std::size_t m_syncedLogCount{0};

    float m_elapsedTime{0.0f};
    unsigned int m_fontSize{16};
};

#endif // GIRIGARA_UI_UICONSOLE_H