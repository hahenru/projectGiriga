#include <UI/UIVerticalLayout.h>

void UIVerticalLayout::update(float dt) {
    if (!isVisible()) return;

    performLayout();
    UIContainer::update(dt);
}

void UIVerticalLayout::performLayout() {
    float currentY = m_paddingTop;
    float availableW = getLocalWidth() - (m_paddingLeft + m_paddingRight);
    bool hasVisibleChild = false; // ★ 1. 新增這行：記錄有沒有任何可見元件

    for (auto& child : m_children) {
        if (!child || !child->isVisible()) continue;

            hasVisibleChild = true; // ★ 2. 只要有元件進來排版，就標記為 true

            if (m_fitWidth && availableW > 0.0f) {
                child->setSize(availableW, child->getLocalHeight());
            }

            child->setPosition(m_paddingLeft, currentY);
            currentY += child->getLocalHeight() + m_spacing;
    }

    // ★ 3. 根據有沒有排到元件，決定高度怎麼算：
    if (hasVisibleChild) {
        // 有排到東西：減掉最後一個元件多加的那次回馬槍 spacing，再加上底部 padding
        m_height = currentY - m_spacing + m_paddingBottom;
    } else {
        // 空容器：高度純粹就是「頂部 padding + 底部 padding」，絕不扣 spacing
        m_height = m_paddingTop + m_paddingBottom;
    }

    if (m_height < 0.0f) m_height = 0.0f;
}
