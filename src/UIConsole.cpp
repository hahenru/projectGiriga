#include <UI/UIConsole.h>
#include <GLFW/glfw3.h>
#include <algorithm>

UIConsole::UIConsole(float x, float y, float width, float height,
                     EventBus& eventBus,
                     ConsoleSystem& consoleSystem, LoggerSystem& logger,
                     IResourceManager& resourceManager,
                     Shader& bgShader)
    : UIContainer(x, y, width, height),
      m_eventBus(eventBus),
      m_consoleSystem(consoleSystem),
      m_logger(logger),
      m_resources(resourceManager),
      m_bgShader(bgShader) {

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
        appendNewLogs();
        m_scrollView->scrollToBottom();
    });

    layoutChildren(width, height);
    bindEvents();
}

UIConsole::UIConsole(float x, float y, float width, float height,
                     EventBus& eventBus,
                     ConsoleSystem& consoleSystem, LoggerSystem& logger,
                     IResourceManager& resourceManager)
    : UIConsole(x, y, width, height, eventBus, consoleSystem, logger, resourceManager,
                resourceManager.get<Shader>("ConsoleBackground")) {}

void UIConsole::setSize(float width, float height) {
    UIContainer::setSize(width, height);
    layoutChildren(width, height);
}

void UIConsole::layoutChildren(float width, float height) {
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

void UIConsole::update(float dt) {
    UIContainer::update(dt);
    if (isVisible()) {
        m_elapsedTime += dt;
    }
    if (m_logger.isDirty()) {
        appendNewLogs();
        m_logger.clearDirty();
    }
}

void UIConsole::render(UIRenderer& renderer) {
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

void UIConsole::bindEvents() {
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
            int newSize = std::clamp(static_cast<int>(m_fontSize) + delta, 12, 42);

            if (static_cast<unsigned int>(newSize) != m_fontSize) {
                m_fontSize = static_cast<unsigned int>(newSize);
                updateFontSizeInPlace(); // ★ 原地更新，完全不限制滾輪次數
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

void UIConsole::appendNewLogs() {
    const auto& logs = m_logger.getLogs();

    // 若 Logger 曾被清空或重置，才重新對齊
    if (logs.size() < m_syncedLogCount) {
        m_scrollView->clearItems();
        m_logItems.clear();
        m_syncedLogCount = 0;
    }

    if (m_syncedLogCount == logs.size()) return;

    const float lineH = static_cast<float>(m_fontSize) * 1.35f;
    float curY = static_cast<float>(m_logItems.size()) * lineH;

    // ★ 只針對新增的區段建立 UIText，舊的完全不動
    for (std::size_t i = m_syncedLogCount; i < logs.size(); ++i) {
        auto* label = m_scrollView->emplaceItem<UIText>(
            0.0f, curY, m_resources, EncodingUtils::utf8ToUtf32(logs[i]), m_fontSize
        );
        label->setColor(0.9f, 0.9f, 0.9f, 1.0f);
        m_logItems.push_back(label);
        curY += lineH;
    }

    m_syncedLogCount = logs.size();
    m_scrollView->fitContentHeight();
    m_scrollView->scrollToBottom();
}

void UIConsole::updateFontSizeInPlace() {
    const float lineH = static_cast<float>(m_fontSize) * 1.35f;
    float curY = 0.0f;

    // ★ 直接改現有 UIText 的字體大小與 Y 座標，零記憶體配置、零字串轉碼
    for (auto* label : m_logItems) {
        label->setFontSize(m_fontSize);
        label->setPosition(0.0f, curY);
        curY += lineH;
    }

    m_scrollView->fitContentHeight();
    m_scrollView->scrollToBottom();
}