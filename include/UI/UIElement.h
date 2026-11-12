#ifndef GIRIGARA_UI_UI_ELEMENT_H
#define GIRIGARA_UI_UI_ELEMENT_H

#include <Core/UUID.h>
#include <Core/Identifiable.h>
#include <Shader/Shader.h>
#include <Script/LuaRef.h>
#include <UI/UIRegistry.h>
#include <string>

enum class UIUnit {
    Pixels,  // 固定像素
    Percent  // 百分比 (100.0f 代表 100%, 0.01f 代表 0.01%)
};

// 前置宣告
class UIRenderer;

class UIElement : public Core::Identifiable {
public:
    virtual ~UIElement();

    UIElement(const UIElement&) = delete;
    UIElement& operator=(const UIElement&) = delete;
    UIElement(UIElement&&) noexcept = default;
    UIElement& operator=(UIElement&&) noexcept = default;

    // 覆寫 setName 以同步更新 UIRegistry 的名稱映射
    void setName(const std::string& name) override;

	virtual void render(UIRenderer& renderer) = 0;
    virtual bool handleMouseMove(double x, double y) = 0;
    virtual bool handleMouseButton(int button, int action, int mods, double x, double y) = 0;

    virtual bool handleKey(int key, int scancode, int action, int mods) {
        (void)key; (void)scancode; (void)action; (void)mods;
        return false;
    }

    virtual void update(float dt);
    void setScriptUpdate(LuaRef callback);

    virtual void setVisible(bool visible);
    bool isVisible() const { return m_visible; }

    // 像素座標與大小設定
    void setPosition(float x, float y);
    void setSize(float width, float height);

    // 百分比座標與大小設定 (精確支援小數點如 0.01f 代表 0.01%)
    void setPositionPercent(float xPercent, float yPercent);
    void setSizePercent(float wPercent, float hPercent);

    // 取得原始數值
    float getRawX() const { return m_x; }
    float getRawY() const { return m_y; }
    float getRawWidth() const { return m_width; }
    float getRawHeight() const { return m_height; }

    UIUnit getPositionUnit() const { return m_posUnit; }
    UIUnit getSizeUnit() const { return m_sizeUnit; }

    // 依據單位換算後的局部像素寬高與座標
    virtual float getLocalWidth() const;
    virtual float getLocalHeight() const;
    float getLocalX() const;
    float getLocalY() const;

    // 沿用原介面名稱：回傳換算後的局部像素值
    float getX() const { return getLocalX(); }
    float getY() const { return getLocalY(); }
    float getWidth() const { return getLocalWidth(); }
    float getHeight() const { return getLocalHeight(); }

    virtual void setFocused(bool focused) { m_isFocused = focused; }
    bool isFocused() const { return m_isFocused; }

    void setParent(UIElement* parent) { m_parent = parent; }
    UIElement* getParent() const { return m_parent; }

    // 遞迴取得全局絕對座標
    float getGlobalX() const;
    float getGlobalY() const;

    // 命中測試
    virtual bool contains(double x, double y) const;
    virtual bool canBringToFront() const { return false; }
	
	// ★ 虛擬標髒介面：供 Container 覆寫以向下傳播
    virtual void markTransformDirty();
    bool isTransformDirty() const { return m_isTransformDirty; }


protected:
    UIElement();
    UIElement(float x, float y, float width, float height, std::string name = "");

    virtual void onUpdate(float dt);

    UIElement* m_parent = nullptr;
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 0.0f;
    float m_height = 0.0f;
	
	// ★ 關鍵：mutable 允許在 const 函式 (getGlobalX/Y) 內部延遲更新快取
    mutable bool  m_isTransformDirty = true;
    mutable float m_cachedGlobalX    = 0.0f;
    mutable float m_cachedGlobalY    = 0.0f;

    void updateGlobalTransform() const;

    UIUnit m_posUnit  = UIUnit::Pixels;
    UIUnit m_sizeUnit = UIUnit::Pixels;

    bool m_visible = true;
    bool m_isFocused = false;

    LuaRef m_scriptUpdate;
};

#endif // GIRIGARA_UI_UI_ELEMENT_H
