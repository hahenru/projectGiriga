#ifndef GIRIGARA_UI_UI_TEXT_H
#define GIRIGARA_UI_UI_TEXT_H

#include <UI/UIElement.h>
#include <Font/FontLoader.h>
#include <memory>
#include <string>

class UIRenderer; // 前置宣告

class UIText : public UIElement {
public:
    // fontSize 預設為 0 代表直接沿用 FontLoader 的原生烘焙大小
    UIText(float x, float y, const FontLoader& font, const std::u32string& text, unsigned int fontSize = 0);
    ~UIText() override = default;

    // 動態修改文字內容（觸發重新烘焙）
    void setText(const std::u32string& text);
    const std::u32string& getText() const { return m_text; }

    // 動態調整字體顯示大小（像素高度）
    void setFontSize(unsigned int fontSize);
    unsigned int getFontSize() const { return m_targetFontSize; }

    // 設定文字顏色 (RGBA)
    void setColor(float r, float g, float b, float a = 1.0f);

    // 實作 UIElement 介面
    void render(UIRenderer& renderer) override;
    bool handleMouseMove(double x, double y) override;
    bool handleMouseButton(int button, int action, int mods, double x, double y) override;

    void fitToParent();

private:
    const FontLoader& m_font;
    std::u32string m_text;
    std::unique_ptr<TextTexture> m_texture = nullptr;
    float m_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    unsigned int m_targetFontSize = 0; // 目標渲染像素字號
    float m_scale = 1.0f;              // 縮放比

    void rebake();
    void updateMetrics();
};

#endif // GIRIGARA_UI_UI_TEXT_H
