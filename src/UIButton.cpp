#include <UI/UIButton.h>
#include <Renderer/UIRenderer.h>
#include <Resource/ResourceManager.h>
#include <GLFW/glfw3.h>

UIButton::UIButton(float x, float y, float width, float height)
    : UIContainer(x, y, width, height) {
    // 按鈕本身不是穿透容器，未被子節點處理的點擊要攔截
    setPassThrough(false);
}

UIText* UIButton::setLabel(ResourceManager& resources, const std::u32string& text,
                           float r, float g, float b, float a,
                           std::string fontName) {
    if (!m_label) {
        // ★ 透過 UIContainer 的 emplaceChild 掛載，傳入 resources
        m_label = emplaceChild<UIText>(0.0f, 0.0f, resources, text, 0, std::move(fontName));
    } else {
        m_label->setText(text);
    }

    m_label->setColor(r, g, b, a);
    updateLabelPosition();
    return m_label;
}

void UIButton::setSize(float width, float height) {
    UIContainer::setSize(width, height);
    updateLabelPosition();
}

void UIButton::updateLabelPosition() {
    if (!m_label) return;

    // 取得換算後真正的局部像素寬高
    float curW = getLocalWidth();
    float curH = getLocalHeight();
    float lblW = m_label->getLocalWidth();
    float lblH = m_label->getLocalHeight();

    float textX = (curW - lblW) * 0.5f;
    float textY = (curH - lblH) * 0.5f;

    m_label->setPosition(textX, textY);
}

void UIButton::update(float dt) {
    UIContainer::update(dt);

    // 當按鈕可能因為百分比換算產生尺寸變動時，保持文字居中
    if (m_label && isTransformDirty()) {
        updateLabelPosition();
    }
}

void UIButton::render(UIRenderer& renderer) {
    if (!isVisible()) return;

    // 1. 決定目前狀態顏色
    const float* activeColor = m_colors.normal;
    if (m_isPressed) {
        activeColor = m_colors.pressed;
    } else if (m_isHovered) {
        activeColor = m_colors.hovered;
    }

    // 2. 繪製按鈕底色
    renderer.drawQuad(getGlobalX(), getGlobalY(), getLocalWidth(), getLocalHeight(), activeColor);

    // 3. 呼叫 UIContainer::render，自動繪製底下的所有子節點（包含 m_label）！
    UIContainer::render(renderer);
}

bool UIButton::handleMouseMove(double x, double y) {
    if (!isVisible()) {
        m_isHovered = false;
        return false;
    }

    // 優先讓子元件有機會處理（若有需要），但通常 UIText handleMouseMove 會回傳 false 穿透
    UIContainer::handleMouseMove(x, y);

    m_isHovered = contains(x, y);
    if (!m_isHovered) {
        m_isPressed = false;
    }

    return m_isHovered;
}

bool UIButton::handleMouseButton(int button, int action, int mods, double x, double y) {
    if (!isVisible()) return false;

    // 1. 優先由子元件處理（例如按鈕裡面內嵌了其它小元件）
    // 但因為 UIText::handleMouseButton 永遠回傳 false，事件會自然漏給按鈕自身處理
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (contains(x, y)) {
                m_isPressed = true;
                return true;
            }
        } else if (action == GLFW_RELEASE) {
            if (m_isPressed && contains(x, y)) {
                m_isPressed = false;
                if (m_onClick) {
                    m_onClick();
                }
                return true;
            }
            m_isPressed = false;
        }
    }
    return false;
}