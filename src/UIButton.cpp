#include <UI/UIButton.h>
#include <Renderer/UIRenderer.h>
#include <Resource/ResourceManager.h>
#include <GLFW/glfw3.h>

UIButton::UIButton(float x, float y, float width, float height)
: UIContainer(x, y, width, height) {
    setPassThrough(false);
}

UIText* UIButton::setLabel(ResourceManager& resources, const std::u32string& text,
                            float r, float g, float b, float a,
                            std::string fontName) {
    if (!m_label) {
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

    // 冷卻計時器倒數
    if (m_cooldownTimer > 0.0f) {
        m_cooldownTimer -= dt;
        if (m_cooldownTimer < 0.0f) {
            m_cooldownTimer = 0.0f;
        }
    }

    if (m_label && isTransformDirty()) {
        updateLabelPosition();
    }
}

void UIButton::render(UIRenderer& renderer) {
    if (!isVisible()) return;

    // 1. 決定目前狀態顏色（冷卻或鎖死時使用 disabled 灰色）
    const float* activeColor = m_colors.normal.data();
    if (m_isLocked || m_cooldownTimer > 0.0f) {
        activeColor = m_colors.disabled.data();
    } else if (m_isPressed) {
        activeColor = m_colors.pressed.data();
    } else if (m_isHovered) {
        activeColor = m_colors.hovered.data();
    }

    // 2. 繪製按鈕底色
    renderer.drawQuad(getGlobalX(), getGlobalY(), getLocalWidth(), getLocalHeight(), activeColor);

    // 3. 繪製子節點
    UIContainer::render(renderer);
}

bool UIButton::handleMouseMove(double x, double y) {
    if (!isVisible()) {
        m_isHovered = false;
        return false;
    }

    UIContainer::handleMouseMove(x, y);

    m_isHovered = contains(x, y);
    if (!m_isHovered || m_isLocked || m_cooldownTimer > 0.0f) {
        m_isPressed = false;
    }

    return m_isHovered;
}

bool UIButton::handleMouseButton(int button, int action, int mods, double x, double y) {
    if (!isVisible()) return false;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (contains(x, y)) {
                // 若被鎖定或冷卻中：吞噬點擊但不進入按壓狀態
                if (m_isLocked || m_cooldownTimer > 0.0f) {
                    return true;
                }
                m_isPressed = true;
                return true;
            }
        } else if (action == GLFW_RELEASE) {
            if (m_isPressed && contains(x, y)) {
                m_isPressed = false;

                // 再次雙重防禦：冷卻中或鎖死則忽略
                if (m_isLocked || m_cooldownTimer > 0.0f) {
                    return true;
                }

                // 進入冷卻
                if (m_cooldownDuration > 0.0f) {
                    m_cooldownTimer = m_cooldownDuration;
                }

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
