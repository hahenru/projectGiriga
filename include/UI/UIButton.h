#ifndef GIRIGARA_UI_UI_BUTTON_H
#define GIRIGARA_UI_UI_BUTTON_H

#include <UI/UIContainer.h>
#include <UI/UIText.h>
#include <Resource/ResourceManager.h>

#include <array>
#include <functional>
#include <string>

using ColorRGBA = std::array<float, 4>;

struct ButtonColors {
    ColorRGBA normal   = {0.25f, 0.55f, 0.95f, 1.0f};
    ColorRGBA hovered  = {0.35f, 0.65f, 1.0f,  1.0f};
    ColorRGBA pressed  = {0.18f, 0.45f, 0.85f, 1.0f};
    ColorRGBA disabled = {0.20f, 0.20f, 0.20f, 0.60f}; // 鎖定與冷卻中的視覺灰色
};

class UIRenderer;

class UIButton : public UIContainer {
public:
    using ClickCallback = std::function<void()>;

    UIButton(float x, float y, float width, float height);
    ~UIButton() override = default;

    void setOnClick(ClickCallback callback) { m_onClick = std::move(callback); }
    void setColors(const ButtonColors& colors) { m_colors = colors; }

    // 冷卻時間設定（單位：秒，0.0f 代表不啟用冷卻）
    void setCooldownDuration(float seconds) { m_cooldownDuration = seconds; }
    bool isOnCooldown() const { return m_cooldownTimer > 0.0f; }

    // 手動/群組互斥鎖定（true 代表鎖死不可點擊）
    void setLocked(bool locked) { m_isLocked = locked; if (locked) m_isPressed = false; }
    bool isLocked() const { return m_isLocked; }

    // 設定按鈕標籤文字與字型
    UIText* setLabel(ResourceManager& resources, const std::u32string& text,
                     float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f,
                     std::string fontName = "default");

    UIText* getLabel() const { return m_label; }

    // 實作與覆寫介面
    void update(float dt) override;
    void render(UIRenderer& renderer) override;
    bool handleMouseMove(double x, double y) override;
    bool handleMouseButton(int button, int action, int mods, double x, double y) override;

    bool isHovered() const { return m_isHovered; }
    bool isPressed() const { return m_isPressed; }

    void setSize(float width, float height);

private:
    bool m_isHovered = false;
    bool m_isPressed = false;
    bool m_isLocked  = false;
    float m_cooldownTimer = 0.0f;
    float m_cooldownDuration = 0.0f;

    ButtonColors m_colors;
    ClickCallback m_onClick = nullptr;

    UIText* m_label = nullptr;
    void updateLabelPosition();
};

#endif // GIRIGARA_UI_UI_BUTTON_H
