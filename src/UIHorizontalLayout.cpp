#include <UI/UIHorizontalLayout.h>

void UIHorizontalLayout::update(float dt) {
    if (!isVisible()) return;

    performLayout();
    UIContainer::update(dt);
}

void UIHorizontalLayout::performLayout() {
    float currentX = m_paddingLeft;
    float availableH = getLocalHeight() - (m_paddingTop + m_paddingBottom);
    bool hasVisibleChild = false;

    for (auto& child : m_children) {
        if (!child || !child->isVisible()) continue;

        hasVisibleChild = true;

        if (m_fitHeight && availableH > 0.0f) {
            child->setSize(child->getLocalWidth(), availableH);
        }

        child->setPosition(currentX, m_paddingTop);
        currentX += child->getLocalWidth() + m_spacing;
    }

    if (hasVisibleChild) {
        // 減去最後一個元件多加的 spacing，再加上右側 padding
        m_width = currentX - m_spacing + m_paddingRight;
    } else {
        // 空容器：寬度純粹就是左右 padding
        m_width = m_paddingLeft + m_paddingRight;
    }

    if (m_width < 0.0f) m_width = 0.0f;
}
