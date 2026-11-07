#ifndef GIRIGARA_UI_UI_PANEL_H
#define GIRIGARA_UI_UI_PANEL_H

#include <UI/UIContainer.h>
#include <array>

class UIRenderer;

class UIPanel : public UIContainer {
public:
    using ColorRGBA = std::array<float, 4>;

    UIPanel(float x, float y, float width, float height);
    ~UIPanel() override = default;

    // 背景顏色設定
    void setBackgroundColor(float r, float g, float b, float a = 1.0f);
    void setBackgroundColor(const ColorRGBA& color) { m_backgroundColor = color; }
    const ColorRGBA& getBackgroundColor() const { return m_backgroundColor; }

    // 邊框顏色與線寬設定
    void setBorderColor(float r, float g, float b, float a = 1.0f);
    void setBorderColor(const ColorRGBA& color) { m_borderColor = color; }
    const ColorRGBA& getBorderColor() const { return m_borderColor; }

    void setBorderWidth(float width) { m_borderWidth = width; }
    float getBorderWidth() const { return m_borderWidth; }

    // 內距設定（Padding: 上、右、下、左）
    void setPadding(float top, float right, float bottom, float left);
    void setPadding(float all);
    float getPaddingTop() const { return m_paddingTop; }
    float getPaddingRight() const { return m_paddingRight; }
    float getPaddingBottom() const { return m_paddingBottom; }
    float getPaddingLeft() const { return m_paddingLeft; }

    // 扣除內距後的可用內容排版區域寬高
    float getContentWidth() const;
    float getContentHeight() const;

    // 覆寫渲染與輸入分發
    void render(UIRenderer& renderer) override;
    bool handleMouseButton(int button, int action, int mods, double x, double y) override;

private:
    ColorRGBA m_backgroundColor = {0.10f, 0.10f, 0.14f, 0.85f}; // 預設深色半透明底色
    ColorRGBA m_borderColor     = {0.35f, 0.35f, 0.40f, 1.00f}; // 預設邊框灰
    float m_borderWidth         = 1.0f;                         // 預設 1 像素邊框

    float m_paddingTop    = 0.0f;
    float m_paddingRight  = 0.0f;
    float m_paddingBottom = 0.0f;
    float m_paddingLeft   = 0.0f;
};

#endif // GIRIGARA_UI_UI_PANEL_H
