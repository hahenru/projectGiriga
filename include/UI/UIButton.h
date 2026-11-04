#ifndef GIRIGARA_UI_UI_BUTTON_H
#define GIRIGARA_UI_UI_BUTTON_H

#include <UI/UIContainer.h>
#include <UI/UIText.h>
#include <functional>
#include <string>

struct ButtonColors {
    float normal[4]  = {0.25f, 0.55f, 0.95f, 1.0f};
    float hovered[4] = {0.35f, 0.65f, 1.0f,  1.0f};
    float pressed[4] = {0.18f, 0.45f, 0.85f, 1.0f};
};

class UIRenderer;

// ★ 繼承 UIContainer，具備完整的子節點管理與標髒傳播能力
class UIButton : public UIContainer {
public:
    using ClickCallback = std::function<void()>;

    UIButton(float x, float y, float width, float height);
    ~UIButton() override = default;

    void setOnClick(ClickCallback callback) { m_onClick = std::move(callback); }
    void setColors(const ButtonColors& colors) { m_colors = colors; }

    // 設定按鈕標籤文字與字型（將 UIText 作為 child 掛載）
    UIText* setLabel(const FontLoader& font, const std::u32string& text,
                     float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

    UIText* getLabel() const { return m_label; }

    // 實作與覆寫介面
    void update(float dt) override;
    void render(UIRenderer& renderer) override;
    bool handleMouseMove(double x, double y) override;
    bool handleMouseButton(int button, int action, int mods, double x, double y) override;

    bool isHovered() const { return m_isHovered; }
    bool isPressed() const { return m_isPressed; }

    // 當按鈕自身尺寸變更時，自動重新計算文字居中
    void setSize(float width, float height);

private:
    bool m_isHovered = false;
    bool m_isPressed = false;
    ButtonColors m_colors;
    ClickCallback m_onClick = nullptr;

    UIText* m_label = nullptr; // 弱引用，所有權在 UIContainer::m_children 裡
    void updateLabelPosition();
};

#endif // GIRIGARA_UI_UI_BUTTON_H