#ifndef GIRIGARA_UI_UI_HORIZONTAL_LAYOUT_H
#define GIRIGARA_UI_UI_HORIZONTAL_LAYOUT_H

#include <UI/UIContainer.h>

class UIHorizontalLayout : public UIContainer {
public:
    // 繼承建構子：支援 UIHorizontalLayout(x, y, width, height)
    using UIContainer::UIContainer;

    ~UIHorizontalLayout() override = default;

    // 設定內邊距 (四周邊界留白)
    void setPadding(float padding) {
        m_paddingLeft = m_paddingRight = m_paddingTop = m_paddingBottom = padding;
    }
    void setPadding(float left, float right, float top, float bottom) {
        m_paddingLeft = left;
        m_paddingRight = right;
        m_paddingTop = top;
        m_paddingBottom = bottom;
    }

    // 設定子元件彼此之間的水平間距
    void setSpacing(float spacing) { m_spacing = spacing; }
    float getSpacing() const { return m_spacing; }

    // 設定是否讓子元件自動拉伸高度填滿整個容器高度
    void setFitHeight(bool fit) { m_fitHeight = fit; }
    bool isFitHeight() const { return m_fitHeight; }

    // 覆寫 update：在每幀更新子節點前，先執行自動幾何重排
    void update(float dt) override;

private:
    float m_paddingLeft   = 10.0f;
    float m_paddingRight  = 10.0f;
    float m_paddingTop    = 10.0f;
    float m_paddingBottom = 10.0f;
    float m_spacing       = 8.0f;
    bool  m_fitHeight     = false; // 若為 true，子元件高度會被自動設為 (容器高度 - 上下Padding)

    void performLayout();
};

#endif // GIRIGARA_UI_UI_HORIZONTAL_LAYOUT_H
