#ifndef GIRIGARA_UI_UISCROLLVIEW_H
#define GIRIGARA_UI_UISCROLLVIEW_H

#include <UI/UIContainer.h>
#include <Renderer/UIRenderer.h>
#include <algorithm>

class UIScrollView : public UIContainer {
public:
    UIScrollView(float x, float y, float width, float height)
        : UIContainer(x, y, width, height) {
        setClipToBounds(true); // 自動剪裁超出內容
        m_contentContainer = emplaceChild<UIContainer>(0.0f, 0.0f, width, height);
    }

    template <typename T, typename... Args>
    T* emplaceItem(Args&&... args) {
        return m_contentContainer->emplaceChild<T>(std::forward<Args>(args)...);
    }

    void clearItems() {
        m_contentContainer->clear();
        m_contentHeight = 0.0f;
        m_scrollY = 0.0f;
        updateContentPosition();
    }

    void scroll(float delta) {
        m_scrollY += delta;
        clampScroll();
        updateContentPosition();
    }

    void scrollToBottom() {
        float maxScroll = std::max(0.0f, m_contentHeight - getHeight());
        m_scrollY = maxScroll;
        updateContentPosition();
    }

    void fitContentHeight() {
        float maxY = 0.0f;
        for (const auto& child : m_contentContainer->getChildren()) {
            float bottom = child->getY() + child->getHeight();
            if (bottom > maxY) maxY = bottom;
        }
        m_contentHeight = maxY;
        m_contentContainer->setSize(getWidth(), m_contentHeight);
        clampScroll();
        updateContentPosition();
    }

    float getContentHeight() const { return m_contentHeight; }

private:
    void clampScroll() {
        float maxScroll = std::max(0.0f, m_contentHeight - getHeight());
        m_scrollY = std::clamp(m_scrollY, 0.0f, maxScroll);
    }

    void updateContentPosition() {
        m_contentContainer->setPosition(0.0f, -m_scrollY);
    }

    UIContainer* m_contentContainer = nullptr;
    float m_scrollY = 0.0f;
    float m_contentHeight = 0.0f;
};

#endif // GIRIGARA_UI_UISCROLLVIEW_H