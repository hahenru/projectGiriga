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

    // 1. 建構子中讓提示符與輸入框初始字級統一使用 m_fontSize（不再寫死 18）
    m_scrollView = emplaceChild<UIScrollView>(0.0f, 0.0f, 100.0f, 100.0f);
    m_inputPrompt = emplaceChild<UIText>(0.0f, 0.0f, m_resources, U">", m_fontSize);
    m_inputPrompt->setColor(0.3f, 1.0f, 0.4f, 1.0f);

    m_inputField = emplaceChild<UIInputField>(
        0.0f, 0.0f, 100.0f, 36.0f,
        m_resources, m_eventBus, m_fontSize
    );

    m_inputField->setOnSubmit([this](const std::string& cmd) {
        m_consoleSystem.executeCommand(cmd);
        m_scrollView->scrollToBottom();
        syncVirtualViewport();
    });

    initItemPool();
    layoutChildren(width, height);
    bindEvents();
}

UIConsole::UIConsole(float x, float y, float width, float height,
         EventBus& eventBus,
         ConsoleSystem& consoleSystem, LoggerSystem& logger,
         IResourceManager& resourceManager)
: UIConsole(x, y, width, height, eventBus, consoleSystem, logger, resourceManager,
resourceManager.get<Shader>("ConsoleBackground")) {}

void UIConsole::initItemPool() {
    m_itemPool.reserve(kPoolCapacity);
    m_poolBoundIndex.assign(kPoolCapacity, kInvalidIndex);

    for (std::size_t i = 0; i < kPoolCapacity; ++i) {
        auto* label = m_scrollView->emplaceItem<UIText>(
0.0f, 0.0f, m_resources, U"", m_fontSize
        );
        label->setColor(0.9f, 0.9f, 0.9f, 1.0f);
        label->setAutoShrink(false);
        label->setVisible(false);
        m_itemPool.push_back(label);
    }
}

void UIConsole::appendWrappedLog(const std::string& utf8Log) {
    std::u32string u32 = EncodingUtils::utf8ToUtf32(utf8Log);
    if (u32.empty()) {
        m_wrappedLines.emplace_back(U"");
        return;
    }

    // 預留右側 16px 安全邊距
    const float maxLineWidth = std::max(60.0f, m_scrollView->getWidth() - 16.0f);
    const float halfWidth = static_cast<float>(m_fontSize) * 0.55f;
    const float fullWidth = static_cast<float>(m_fontSize) * 1.02f;

    std::u32string currentLine;
    float currentWidth = 0.0f;

    for (char32_t ch : u32) {
        if (ch == U'\r') continue;
        if (ch == U'\n') {
m_wrappedLines.push_back(std::move(currentLine));
currentLine.clear();
currentWidth = 0.0f;
continue;
        }

        float charW = (ch < 0x80) ? halfWidth : fullWidth;
        if (!currentLine.empty() && currentWidth + charW > maxLineWidth) {
m_wrappedLines.push_back(std::move(currentLine));
currentLine.clear();
currentWidth = 0.0f;
        }

        currentLine.push_back(ch);
        currentWidth += charW;
    }

    if (!currentLine.empty()) {
        m_wrappedLines.push_back(std::move(currentLine));
    }
}

void UIConsole::rebuildWrappedLines() {
    const auto& logs = m_logger.getLogs();
    m_wrappedLines.clear();
    m_wrappedLines.reserve(logs.size());

    for (const auto& log : logs) {
        appendWrappedLog(log);
    }

    m_syncedRawLogCount = logs.size();
    std::fill(m_poolBoundIndex.begin(), m_poolBoundIndex.end(), kInvalidIndex);

    const float lineH = static_cast<float>(m_fontSize) * 1.35f;
    m_scrollView->setContentHeight(static_cast<float>(m_wrappedLines.size()) * lineH);
    m_scrollView->scrollToBottom();
}

void UIConsole::setSize(float width, float height) {
    UIContainer::setSize(width, height);
    layoutChildren(width, height);
}

// 2. layoutChildren 改為依據 m_fontSize 動態計算輸入框高度與垂直置中對齊
void UIConsole::layoutChildren(float width, float height) {
    float padding = 15.0f;
    float fontF = static_cast<float>(m_fontSize);

    // ★ 輸入框高度與提示符寬度隨字級等比放大
    float inputH = std::max(36.0f, fontF * 1.6f + 12.0f);
    float promptW = std::max(24.0f, fontF * 0.9f);
    float inputY = height - inputH - padding * 0.5f;

    if (m_scrollView) {
        m_scrollView->setPosition(padding, padding);
        m_scrollView->setSize(width - padding * 2.0f, height - inputH - padding * 2.5f);
    }

    if (m_inputPrompt) {
        // 讓 ">" 提示符在輸入框高度內垂直微調對齊
        float promptOffsetY = (inputH - fontF) * 0.45f;
        m_inputPrompt->setPosition(padding, inputY + promptOffsetY);
    }

    if (m_inputField) {
        m_inputField->setPosition(padding + promptW, inputY);
        m_inputField->setSize(width - padding * 2.0f - promptW, inputH);
    }

    if (isVisible()) {
        rebuildWrappedLines();
    }
    syncVirtualViewport();
}

void UIConsole::update(float dt) {
    UIContainer::update(dt);
    if (!isVisible()) return;

    m_elapsedTime += dt;

    // ★ 2. 有新日誌進來時，重建折行快取（內部會自動更新 ContentHeight 並捲動到底部）
    if (m_logger.isDirty()) {
        m_logger.clearDirty();
        rebuildWrappedLines();
    }

    syncVirtualViewport();
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

        // 3. bindEvents 中的 Ctrl + 滾輪縮放：提高上限至 72，並同步放大提示符與輸入框
        if (isCtrlDown) {
            int delta = (e.yoffset > 0.0) ? 2 : -2; // ★ 4K 螢幕每次跳 2px 縮放手感更順
            int newSize = std::clamp(static_cast<int>(m_fontSize) + delta, 12, 72); // ★ 上限拉高到 72

            if (static_cast<unsigned int>(newSize) != m_fontSize) {
                m_fontSize = static_cast<unsigned int>(newSize);

                if (m_inputPrompt) {
                    m_inputPrompt->setFontSize(m_fontSize);
                }
                if (m_inputField) {
                    m_inputField->setFontSize(m_fontSize);
                }

                // 重新計算輸入框高度、捲動區高度與自動折行
                layoutChildren(getWidth(), getHeight());
            }
        } else {
            m_scrollView->scroll(static_cast<float>(-e.yoffset * 24.0f));
            syncVirtualViewport();
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

void UIConsole::syncVirtualViewport() {
    if (!m_scrollView) return;

    // ★ 4. 改為讀取已經切好行的 m_wrappedLines（不再每次重跑 utf8ToUtf32）
    const float lineH = static_cast<float>(m_fontSize) * 1.35f;

    const float scrollY = std::max(0.0f, m_scrollView->getScrollY());
    const float viewH = m_scrollView->getHeight();

    const std::size_t firstIdx = static_cast<std::size_t>(scrollY / lineH);
    const std::size_t visibleCount = std::min(
        kPoolCapacity,
        static_cast<std::size_t>(viewH / lineH) + 2
    );

    for (std::size_t i = 0; i < kPoolCapacity; ++i) {
        const std::size_t logIdx = firstIdx + i;
        UIText* label = m_itemPool[i];

        if (i < visibleCount && logIdx < m_wrappedLines.size()) {
label->setVisible(true);
label->setFontSize(m_fontSize);
label->setPosition(0.0f, static_cast<float>(logIdx) * lineH);

if (m_poolBoundIndex[i] != logIdx) {
    label->setText(m_wrappedLines[logIdx]);
    m_poolBoundIndex[i] = logIdx;
}
        } else {
label->setVisible(false);
        }
    }
}

void UIConsole::setVisible(bool visible) {
    UIContainer::setVisible(visible);
    if (visible) {
        // ★ 5. 打開 Console 時，一次性把累積的日誌折行並捲動到底部
        m_logger.clearDirty();
        rebuildWrappedLines();
        syncVirtualViewport();
    }
}
