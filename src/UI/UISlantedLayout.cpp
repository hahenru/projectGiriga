#include <UI/UISlantedLayout.h>
#include <algorithm>

void UISlantedLayout::update(float dt) {
    if (!isVisible()) return;

    performLayout();
    UIContainer::update(dt);
}

void UISlantedLayout::performLayout() {
    float currentY = m_paddingTop;
    float currentX = m_paddingLeft;
    bool hasVisibleChild = false;

    float maxX = 0.0f;

    for (auto& child : m_children) {
        if (!child || !child->isVisible()) continue;

        hasVisibleChild = true;

        // 設定階梯式斜向座標
        child->setPosition(currentX, currentY);

        // 追蹤最右側邊界，供計算容器總寬度
        maxX = std::max(maxX, currentX + child->getLocalWidth());

        // X 軸向右/左位移，Y 軸向下推進
        currentX += m_slantOffsetX;
        currentY += child->getLocalHeight() + m_spacing;
    }

    if (hasVisibleChild) {
        m_height = currentY - m_spacing + m_paddingTop;
        m_width = maxX + m_paddingLeft;
    } else {
        m_height = m_paddingTop * 2.0f;
        m_width = m_paddingLeft * 2.0f;
    }
}
