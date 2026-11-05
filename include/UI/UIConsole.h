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
#include <Resource/ResourceManager.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <algorithm>

class UIConsole : public UIContainer {
public:
    UIConsole(float x, float y, float width, float height,
              EventBus& eventBus,
              ConsoleSystem& consoleSystem, LoggerSystem& logger,
              ResourceManager& resourceManager)
        : UIContainer(x, y, width, height),
          m_eventBus(eventBus),
          m_consoleSystem(consoleSystem),
          m_logger(logger),
          m_resources(resourceManager),
          m_bgShader(resourceManager.loadShader(
              "ConsoleBackground",
              "./assets/shaders/console_bg.vert",
              "./assets/shaders/console_bg.frag"
          )) {

        setVisible(false);
        setPassThrough(false);

        m_scrollView = emplaceChild<UIScrollView>(0.0f, 0.0f, 100.0f, 100.0f);
        m_inputPrompt = emplaceChild<UIText>(0.0f, 0.0f, m_resources, U">", 18);
        m_inputPrompt->setColor(0.3f, 1.0f, 0.4f, 1.0f);

        m_inputField = emplaceChild<UIInputField>(
            0.0f, 0.0f, 100.0f, 36.0f,
            m_resources, m_eventBus, 18
        );

        m_inputField->setOnSubmit([this](const std::string& cmd) {
            m_consoleSystem.executeCommand(cmd);
            refreshLogs();
            m_scrollView->scrollToBottom();
        });

        layoutChildren(width, height);
        bindEvents();
    }

    void setSize(float width, float height) {
        UIContainer::setSize(width, height);
        layoutChildren(width, height);
    }

    void layoutChildren(float width, float height) {
        float padding = 15.0f;
        float inputH = 36.0f;
        float promptW = 24.0f;

        if (m_scrollView) {
            m_scrollView->setPosition(padding, padding);
            m_scrollView->setSize(width - padding * 2.0f, height - inputH - padding * 2.5f);
        }

        if (m_inputPrompt) {
            m_inputPrompt->setPosition(padding, height - inputH - padding * 0.5f);
        }

        if (m_inputField) {
            m_inputField->setPosition(padding + promptW, height - inputH - padding * 0.5f);
            m_inputField->setSize(width - padding * 2.0f - promptW, inputH);
        }
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

    void render(UIRenderer& renderer) override {
        if (!isVisible()) return;

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

        renderer.beginCurrent();
        UIContainer::render(renderer);
    }

    UIInputField* getInputField() { return m_inputField; }

private:
    void bindEvents() {
        m_eventBus.subscribePriority<MouseScrolledEvent>([this](MouseScrolledEvent& e) {
            if (!this->isVisible()) return;
            e.isHandled = true;

            bool isCtrlDown = (e.mods & GLFW_MOD_CONTROL) != 0;
            if (!isCtrlDown) {
                GLFWwindow* win = glfwGetCurrentContext();
                if (win) {
                    isCtrlDown = (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                                  glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);
                }
            }

            if (isCtrlDown) {
                int delta = (e.yoffset > 0.0) ? 1 : -1;
                int newSize = static_cast<int>(m_fontSize) + delta;
                newSize = std::clamp(newSize, 12, 42);

                if (static_cast<unsigned int>(newSize) != m_fontSize) {
                    m_fontSize = static_cast<unsigned int>(newSize);
                    refreshLogs();
                }
            } else {
                m_scrollView->scroll(static_cast<float>(-e.yoffset * 24.0f));
            }
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
        float lineH = static_cast<float>(m_fontSize) * 1.35f;

        for (const auto& log : logs) {
            auto* label = m_scrollView->emplaceItem<UIText>(
                0.0f, curY, m_resources, EncodingUtils::utf8ToUtf32(log), m_fontSize
            );
            label->setColor(0.9f, 0.9f, 0.9f, 1.0f);
            curY += lineH;
        }
        m_scrollView->fitContentHeight();
        m_scrollView->scrollToBottom();
    }

    EventBus& m_eventBus;
    ConsoleSystem& m_consoleSystem;
    LoggerSystem& m_logger;
    ResourceManager& m_resources;
    Shader& m_bgShader;

    UIScrollView* m_scrollView{nullptr};
    UIInputField* m_inputField{nullptr};
    UIText* m_inputPrompt{nullptr};
    float m_elapsedTime{0.0f};

    unsigned int m_fontSize{16};
};

#endif // GIRIGARA_UI_UICONSOLE_H