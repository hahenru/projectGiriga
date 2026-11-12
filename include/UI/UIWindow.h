#ifndef GIRIGARA_UI_UI_WINDOW_H
#define GIRIGARA_UI_UI_WINDOW_H

#include <UI/UIElement.h>
#include <UI/UIContainer.h>
#include <UI/UIText.h>
#include <UI/UIButton.h>
#include <Resource/IResourceManager.h>

#include <memory>
#include <string>

class UIWindow : public UIElement {
public:
    // 方式 1：字串構造，支援指定標題字號 (預設 0 使用原字型大小)
	UIWindow(float x, float y, float width, float height,
		IResourceManager& resources, const std::u32string& title,
		unsigned int titleFontSize = 0, std::string fontName = "default");

    // 方式 2：直接傳入外部構造好的 UIText (支援移動語意)
    UIWindow(float x, float y, float width, float height,
		std::unique_ptr<UIText> titleText);

    ~UIWindow() override = default;

    template <typename T, typename... Args>
    T* emplaceContent(float relX, float relY, Args&&... args) {
        auto child = std::make_unique<T>(relX, relY, std::forward<Args>(args)...);
        T* ptr = child.get();
        m_container.addChild(std::move(child)); // 由 addChild 自動完成 child->setParent(&m_container)[cite: 25]
        return ptr;
    }

    void setPosition(float x, float y);

    // 支援動態更換整個標題物件
    void setTitle(std::unique_ptr<UIText> titleText);

    // 存取標題文字指標以利動態調整
    UIText* getTitleText() const { return m_titleText.get(); }

    void setCloseOnEsc(bool enable) { m_closeOnEsc = enable; }
    bool isCloseOnEscEnabled() const { return m_closeOnEsc; }

    void render(UIRenderer& renderer) override;
    bool handleMouseMove(double x, double y) override;
    bool handleMouseButton(int button, int action, int mods, double x, double y) override;
    bool handleKey(int key, int scancode, int action, int mods) override;
    bool isDragging() const { return m_isDragging; }
    // ★ 主動將視窗位置約束在螢幕可見區域內
    void clampPosition(float screenW, float screenH);
    bool canBringToFront() const override { return true; }
	void markTransformDirty() override;

private:
    float m_titleBarHeight = 35.0f;
    bool m_isDragging = false;
    bool m_closeOnEsc = true;
    double m_dragOffsetX = 0.0;
    double m_dragOffsetY = 0.0;

    float m_bodyColor[4]     = {0.15f, 0.15f, 0.18f, 0.95f};
    float m_titleBarColor[4] = {0.22f, 0.22f, 0.26f, 1.0f};

    std::unique_ptr<UIText> m_titleText = nullptr;
    std::unique_ptr<UIButton> m_closeButton = nullptr;
    UIContainer m_container;

    bool isInsideTitleBar(double x, double y) const;
    void initCloseButton();
};

#endif // GIRIGARA_UI_UI_WINDOW_H
