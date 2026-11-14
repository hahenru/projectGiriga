#include <UI/UIPanel.h>
#include <Renderer/UIRenderer.h>
#include <algorithm>

UIPanel::UIPanel(float x, float y, float width, float height)
: UIContainer(x, y, width, height) {
    // 實體面板預設不穿透，阻擋滑鼠穿透到底層場景
    setPassThrough(false);
}

void UIPanel::setBackgroundColor(float r, float g, float b, float a) {
    m_backgroundColor = {r, g, b, a};
}

void UIPanel::setBorderColor(float r, float g, float b, float a) {
    m_borderColor = {r, g, b, a};
}

void UIPanel::setPadding(float top, float right, float bottom, float left) {
    m_paddingTop = top;
    m_paddingRight = right;
    m_paddingBottom = bottom;
    m_paddingLeft = left;
}

void UIPanel::setPadding(float all) {
    setPadding(all, all, all, all);
}

float UIPanel::getContentWidth() const {
    return std::max(0.0f, getLocalWidth() - m_paddingLeft - m_paddingRight);
}

float UIPanel::getContentHeight() const {
    return std::max(0.0f, getLocalHeight() - m_paddingTop - m_paddingBottom);
}

void UIPanel::render(UIRenderer& renderer) {
    if (!isVisible()) return;

    float gx = getGlobalX();
    float gy = getGlobalY();
    float gw = getLocalWidth();
    float gh = getLocalHeight();

    // 1. 繪製面板主背景（當 Alpha > 0 時）
    if (m_backgroundColor[3] > 0.0f) {
        renderer.drawQuad(gx, gy, gw, gh,
                          m_backgroundColor[0], m_backgroundColor[1],
                          m_backgroundColor[2], m_backgroundColor[3]);
    }

    // 2. 繪製四邊實體邊框
    if (m_borderWidth > 0.0f && m_borderColor[3] > 0.0f) {
        float bw = m_borderWidth;
        float br = m_borderColor[0], bg = m_borderColor[1], bb = m_borderColor[2], ba = m_borderColor[3];

        // 頂部與底部邊框
        renderer.drawQuad(gx, gy, gw, bw, br, bg, bb, ba);
        renderer.drawQuad(gx, gy + gh - bw, gw, bw, br, bg, bb, ba);

        // 左側與右側邊框（避開角落重複重疊，扣除上下線寬）
        if (gh > 2.0f * bw) {
            renderer.drawQuad(gx, gy + bw, bw, gh - (2.0f * bw), br, bg, bb, ba);
            renderer.drawQuad(gx + gw - bw, gy + bw, bw, gh - (2.0f * bw), br, bg, bb, ba);
        }
    }

    // 3. 呼叫 UIContainer::render 自動遞迴繪製所有相對掛載在裡面的子元件
    UIContainer::render(renderer);
}

bool UIPanel::handleMouseButton(int button, int action, int mods, double x, double y) {
    if (!isVisible()) return false;

    // 1. 優先交由子元件處理（例如裡面的按鈕、輸入框）
    if (UIContainer::handleMouseButton(button, action, mods, x, y)) {
        return true;
    }

    // 2. 如果子元件都不吃這個點擊：
    // 若為穿透模式，放行到底層
    if (isPassThrough()) {
        return false;
    }

    // 非穿透模式下，若點擊命中本體矩形，由面板吞噬該事件，阻斷穿透
    return contains(x, y);
}
