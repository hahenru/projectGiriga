#include <UI/UIWindow.h>
#include <GLFW/glfw3.h>
#include <algorithm>

UIWindow::UIWindow(float x, float y, float width, float height,
                   const FontLoader& font, const std::u32string& title,
                   unsigned int titleFontSize)
: UIElement(x, y, width, height) {
    // 1. 內容容器綁定：位置相對為 (0, 0)，尺寸同步視窗大小
    m_container.setParent(this);
    m_container.setPosition(0.0f, 0.0f);
    m_container.setSize(width, height);

    // 2. 標題文字持有相對座標 (Local: 10, yOffset)
    m_titleText = std::make_unique<UIText>(10.0f, 0.0f, font, title, titleFontSize);
    m_titleText->setParent(this);
    float textY = (m_titleBarHeight - m_titleText->getHeight()) * 0.5f;
    m_titleText->setPosition(10.0f, textY);

    initCloseButton();
}

UIWindow::UIWindow(float x, float y, float width, float height,
                   std::unique_ptr<UIText> titleText)
: UIElement(x, y, width, height), m_titleText(std::move(titleText)) {
    // 1. 內容容器綁定：位置相對為 (0, 0)，尺寸同步視窗大小
    m_container.setParent(this);
    m_container.setPosition(0.0f, 0.0f);
    m_container.setSize(width, height);

    if (m_titleText) {
        m_titleText->setParent(this);
        float textY = (m_titleBarHeight - m_titleText->getHeight()) * 0.5f;
        m_titleText->setPosition(10.0f, textY);
    }
    initCloseButton();
}

void UIWindow::initCloseButton() {
    float btnSize = 20.0f;
    float btnMargin = (m_titleBarHeight - btnSize) * 0.5f;

    // 關閉按鈕使用相對座標
    m_closeButton = std::make_unique<UIButton>(
        m_width - btnSize - btnMargin,
        btnMargin,
        btnSize, btnSize
    );
    m_closeButton->setParent(this);
    m_closeButton->setColors({
        {0.8f, 0.2f, 0.2f, 1.0f},
        {1.0f, 0.3f, 0.3f, 1.0f},
        {0.6f, 0.1f, 0.1f, 1.0f}
    });
    m_closeButton->setOnClick([this]() {
        setVisible(false);
    });
}

void UIWindow::setTitle(std::unique_ptr<UIText> titleText) {
    m_titleText = std::move(titleText);
    if (m_titleText) {
        m_titleText->setParent(this);
        m_titleText->setPosition(10.0f, (m_titleBarHeight - m_titleText->getHeight()) * 0.5f);
    }
}

void UIWindow::setPosition(float x, float y) {
    m_x = x;
    m_y = y;
	markTransformDirty(); // ★ 補上標髒
}

bool UIWindow::isInsideTitleBar(double x, double y) const {
    float gx = getGlobalX();
    float gy = getGlobalY();
    return (x >= gx && x <= gx + m_width &&
    y >= gy && y <= gy + m_titleBarHeight);
}

void UIWindow::render(const Shader& shader) {
    if (!m_visible) return;

    constexpr GLint LOC_MODEL    = 1;
    constexpr GLint LOC_UI_COLOR = 2;
    constexpr GLint LOC_IS_TEXT  = 3;

    glProgramUniform1i(shader.getID(), LOC_IS_TEXT, 0);

    float gx = getGlobalX();
    float gy = getGlobalY();

    // 1. 繪製視窗本體 (Body)
    glProgramUniform4fv(shader.getID(), LOC_UI_COLOR, 1, m_bodyColor);
    float bodyModel[16] = {
        m_width, 0.0f,     0.0f, 0.0f,
        0.0f,    m_height, 0.0f, 0.0f,
        0.0f,    0.0f,     1.0f, 0.0f,
        gx,      gy,       0.0f, 1.0f
    };
    glProgramUniformMatrix4fv(shader.getID(), LOC_MODEL, 1, GL_FALSE, bodyModel);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2. 繪製標題列 (Title Bar)
    glProgramUniform4fv(shader.getID(), LOC_UI_COLOR, 1, m_titleBarColor);
    float titleModel[16] = {
        m_width, 0.0f,             0.0f, 0.0f,
        0.0f,    m_titleBarHeight, 0.0f, 0.0f,
        0.0f,    0.0f,             1.0f, 0.0f,
        gx,      gy,               0.0f, 1.0f
    };
    glProgramUniformMatrix4fv(shader.getID(), LOC_MODEL, 1, GL_FALSE, titleModel);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 3. 繪製標題文字與關閉按鈕
    if (m_titleText) {
        m_titleText->render(shader);
    }
    if (m_closeButton) {
        m_closeButton->render(shader);
    }

    // 4. 繪製內部內容容器
    m_container.render(shader);
}

void UIWindow::clampPosition(float screenW, float screenH) {
    // 1. 完全不能跑到整個軟體視窗上面: Y >= 0.0f
    // 2. titleBar 最多只能有 50% 跑到軟體視窗下面: Y <= screenH - (0.5f * m_titleBarHeight)
    float minY = 0.0f;
    float maxY = screenH - (m_titleBarHeight * 0.5f);
    m_y = std::clamp(m_y, minY, maxY);

    // 3. titleBar 最多只能有 70% 跑到軟體視窗左右邊外面 (留在畫面內的寬度 >= 30%)
    float minX = -0.70f * m_width;
    float maxX = screenW - (0.30f * m_width);
    m_x = std::clamp(m_x, minX, maxX);
}

bool UIWindow::handleMouseMove(double x, double y) {
    if (!m_visible) return false;

    // 拖曳處理
    if (m_isDragging) {
        m_x = static_cast<float>(x - m_dragOffsetX);
        m_y = static_cast<float>(y - m_dragOffsetY);

		/**
		* TODO: 之後要把這個hard code移除掉，滑鼠的檢查，直接走Event System.
		*/
        GLFWwindow* currentContext = glfwGetCurrentContext();
        int winW = 800, winH = 600;
        if (currentContext) {
            glfwGetFramebufferSize(currentContext, &winW, &winH);
        }

        clampPosition(static_cast<float>(winW), static_cast<float>(winH));
		
		// ★ 核心修復：座標更新後立即標記髒，通知渲染管線與所有子元件重算全局座標！
        markTransformDirty();
		
        return true;
    }

    if (m_closeButton && m_closeButton->handleMouseMove(x, y)) {
        return true;
    }

    if (m_container.handleMouseMove(x, y)) {
        return true;
    }

    return contains(x, y);
}

bool UIWindow::handleMouseButton(int button, int action, int mods, double x, double y) {
    if (!m_visible) return false;

    if (m_closeButton && m_closeButton->handleMouseButton(button, action, mods, x, y)) {
        return true;
    }

    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
        if (isInsideTitleBar(x, y)) {
            m_isDragging = true;
            m_dragOffsetX = x - m_x;
            m_dragOffsetY = y - m_y;
            return true;
        }

        if (m_container.handleMouseButton(button, action, mods, x, y)) {
            return true;
        }

        if (contains(x, y)) {
            return true;
        }
    } else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT) {
        if (m_isDragging) {
            m_isDragging = false;
            return true;
        }
        if (m_container.handleMouseButton(button, action, mods, x, y)) {
            return true;
        }
    }

    return false;
}

bool UIWindow::handleKey(int key, int /*scancode*/, int action, int /*mods*/) {
    if (!m_visible) return false;

    if (m_isFocused && isCloseOnEscEnabled() && key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        setVisible(false);
        return true;
    }

    return m_container.handleKey(key, 0, action, 0);
}

void UIWindow::markTransformDirty() {
    UIElement::markTransformDirty(); // 先標記自己

    // 同步標記內部持有的一體化元件
    if (m_titleText)   m_titleText->markTransformDirty();
    if (m_closeButton) m_closeButton->markTransformDirty();
    m_container.markTransformDirty();
}