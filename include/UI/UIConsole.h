#ifndef GIRIGARA_UI_UICONSOLE_H
#define GIRIGARA_UI_UICONSOLE_H

#include <UI/UIContainer.h>
#include <UI/UIScrollView.h>
#include <UI/UIInputField.h>
#include <UI/UIText.h>
#include <System/LoggerSystem.h>
#include <System/ConsoleSystem.h>
#include <Font/FontLoader.h>
#include <Event/EventBus.h>
#include <Renderer/UIRenderer.h>
#include <Resource/ResourceManager.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

class UIConsole : public UIContainer {
public:
    UIConsole(float x, float y, float width, float height,
              FontLoader& font, EventBus& eventBus,
              ConsoleSystem& consoleSystem, LoggerSystem& logger,
              ResourceManager& resourceManager)
        : UIContainer(x, y, width, height),
          m_font(font),
          m_eventBus(eventBus),
          m_consoleSystem(consoleSystem),
          m_logger(logger),
          m_bgShader(resourceManager.loadShader(
              "ConsoleBackground",
              "./assets/shaders/console_bg.vert",
              "./assets/shaders/console_bg.frag"
          )) {

        setVisible(false);

        // 1. 建立通用滾動日誌區 (UIScrollView)
        float inputH = 36.0f;
        m_scrollView = emplaceChild<UIScrollView>(15.0f, 15.0f, width - 30.0f, height - inputH - 25.0f);

        // 2. 建立通用輸入提示符與輸入框 (UIInputField)
        float promptW = 24.0f;
        m_inputPrompt = emplaceChild<UIText>(15.0f, height - inputH, m_font, U"> ", 18);
        m_inputPrompt->setColor(0.3f, 1.0f, 0.4f, 1.0f);

        m_inputField = emplaceChild<UIInputField>(
            15.0f + promptW, height - inputH,
            width - 30.0f - promptW, inputH,
            m_font, m_eventBus, 18
        );

        // 3. 輸入完成後執行指令的回呼
        m_inputField->setOnSubmit([this](const std::string& cmd) {
            m_consoleSystem.executeCommand(cmd);
            refreshLogs();
            m_scrollView->scrollToBottom();
        });

        bindEvents();
    }

    void update(float dt) override {
        UIContainer::update(dt); 
        if (isVisible()) {
            m_elapsedTime += dt;
        }
        if (m_logger.isDirty()) {
            refreshLogs();
            m_logger.clearDirty();
        }
    }

    // 移除多餘的 UIConsole:: 宣告限定詞
    void render(UIRenderer& renderer) override {
        if (!isVisible()) return;

        // 1. 背景專用 Shader 繪製 (使用 UIRenderer 提供的 Projection 與 QuadMesh)
        m_bgShader.use();
        GLuint bgProg = m_bgShader.getID();

        glProgramUniformMatrix4fv(bgProg, 0, 1, GL_FALSE, renderer.getProjectionMatrix());
        float model[16] = {
            m_width, 0.0f,     0.0f, 0.0f,
            0.0f,    m_height, 0.0f, 0.0f,
            0.0f,    0.0f,     1.0f, 0.0f,
            getGlobalX(), getGlobalY(), 0.0f, 1.0f
        };
        glProgramUniformMatrix4fv(bgProg, 1, 1, GL_FALSE, model);
        glProgramUniform1f(bgProg, 2, m_elapsedTime);
        glProgramUniform2f(bgProg, 3, m_width, m_height);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 2. 切回標準 UI 管線並遞迴渲染子元件 (ScrollView, InputField, Prompt)
        renderer.beginCurrent();
        UIContainer::render(renderer);
    }

    UIInputField* getInputField() { return m_inputField; }

private:
    void bindEvents() {
        m_eventBus.subscribePriority<MouseScrolledEvent>([this](MouseScrolledEvent& e) {
            if (!this->isVisible()) return; 
            e.isHandled = true;
            m_scrollView->scroll(static_cast<float>(-e.yoffset * 24.0f));
        });

        m_eventBus.subscribePriority<KeyEvent>([this](KeyEvent& e) {
            if (!this->isVisible()) return;
            if (e.key == GLFW_KEY_ESCAPE && e.action == GLFW_PRESS) {
                this->setVisible(false);
                e.isHandled = true;
            }
        });
    }

    void refreshLogs() {
        m_scrollView->clearItems();
        const auto& logs = m_logger.getLogs();
        float curY = 0.0f;
        float lineH = 22.0f;

        for (const auto& log : logs) {
            auto* label = m_scrollView->emplaceItem<UIText>(0.0f, curY, m_font, EncodingUtils::utf8ToUtf32(log), 16);
            label->setColor(0.9f, 0.9f, 0.9f, 1.0f);
            curY += lineH;
        }
        m_scrollView->fitContentHeight();
        m_scrollView->scrollToBottom();
    }

    FontLoader& m_font;
    EventBus& m_eventBus;
    ConsoleSystem& m_consoleSystem;
    LoggerSystem& m_logger;
    Shader& m_bgShader;

    UIScrollView* m_scrollView{nullptr};
    UIInputField* m_inputField{nullptr};
    UIText* m_inputPrompt{nullptr};
    float m_elapsedTime{0.0f};
};

#endif // GIRIGARA_UI_UICONSOLE_H