#include <UI/UIElement.h>
#include <iostream>

UIElement::UIElement() : Core::Identifiable() {
    UIRegistry::getInstance().registerElement(m_uuid, "", this);
}

UIElement::UIElement(float x, float y, float width, float height, std::string name)
: Core::Identifiable(std::move(name)),
m_x(x), m_y(y), m_width(width), m_height(height) {
    UIRegistry::getInstance().registerElement(m_uuid, m_name, this);
}

UIElement::~UIElement() {
    // 物件銷毀時自動脫離全域註冊表
    UIRegistry::getInstance().unregisterElement(m_uuid, m_name);
}

void UIElement::setName(const std::string& name) {
    // 修正：註銷時傳入真實自身 m_uuid，確保正確移除舊鍵值映射
    UIRegistry::getInstance().unregisterElement(m_uuid, m_name);
    Identifiable::setName(name);
    UIRegistry::getInstance().registerElement(m_uuid, m_name, this);
}

void UIElement::setVisible(bool visible) {
    m_visible = visible;
    if (!visible) {
        m_isFocused = false;
    }
}

// 在數值變更時標髒（包含百分比與固定像素）
void UIElement::setPosition(float x, float y) {
    m_x = x;
    m_y = y;
    m_posUnit = UIUnit::Pixels;
    markTransformDirty();
}

void UIElement::setSize(float width, float height) {
    m_width = width;
    m_height = height;
    m_sizeUnit = UIUnit::Pixels;
    markTransformDirty();
}

void UIElement::setPositionPercent(float xPercent, float yPercent) {
    m_x = xPercent;
    m_y = yPercent;
    m_posUnit = UIUnit::Percent;
    markTransformDirty();
}

void UIElement::setSizePercent(float wPercent, float hPercent) {
    m_width = wPercent;
    m_height = hPercent;
    m_sizeUnit = UIUnit::Percent;
    markTransformDirty();
}

float UIElement::getLocalWidth() const {
    if (m_sizeUnit == UIUnit::Percent && m_parent) {
        return m_parent->getLocalWidth() * (m_width / 100.0f);
    }
    return m_width;
}

float UIElement::getLocalHeight() const {
    if (m_sizeUnit == UIUnit::Percent && m_parent) {
        return m_parent->getLocalHeight() * (m_height / 100.0f);
    }
    return m_height;
}

float UIElement::getLocalX() const {
    if (m_posUnit == UIUnit::Percent && m_parent) {
        return m_parent->getLocalWidth() * (m_x / 100.0f);
    }
    return m_x;
}

float UIElement::getLocalY() const {
    if (m_posUnit == UIUnit::Percent && m_parent) {
        return m_parent->getLocalHeight() * (m_y / 100.0f);
    }
    return m_y;
}

float UIElement::getGlobalX() const {
    updateGlobalTransform();
    return m_cachedGlobalX;
}

float UIElement::getGlobalY() const {
    updateGlobalTransform();
    return m_cachedGlobalY;
}

bool UIElement::contains(double x, double y) const {
    float gx = getGlobalX();
    float gy = getGlobalY();
    float gw = getLocalWidth();
    float gh = getLocalHeight();

    return (x >= gx && x <= gx + gw &&
    y >= gy && y <= gy + gh);
}

void UIElement::update(float dt) {
    onUpdate(dt);
}

void UIElement::setScriptUpdate(LuaRef callback) {
    m_scriptUpdate = std::move(callback);
}

void UIElement::onUpdate(float dt) {
    if (m_scriptUpdate.isValid()) {
        lua_State* L = m_scriptUpdate.getState();
        // 修正：記錄進入前的棧深，保證高頻調用時絕對棧平衡
        int top = lua_gettop(L);

        m_scriptUpdate.push();
        lua_pushlightuserdata(L, this);
        lua_pushnumber(L, static_cast<lua_Number>(dt));

        if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            std::cerr << "[Lua Script Error in Update] " << (err ? err : "Unknown") << "\n";
        }
        // 無論腳本成功或拋出錯誤，強制將棧指標恢復至原點
        lua_settop(L, top);
    }
}

// 核心快取計算：只在需要時更新，且相容原本的 getLocalX()
void UIElement::updateGlobalTransform() const {
    if (m_isTransformDirty) {
        float parentGX = m_parent ? m_parent->getGlobalX() : 0.0f;
        float parentGY = m_parent ? m_parent->getGlobalY() : 0.0f;

        // 直接調用 getLocalX() / getLocalY()，百分比計算完全相容
        m_cachedGlobalX = getLocalX() + parentGX;
        m_cachedGlobalY = getLocalY() + parentGY;

        m_isTransformDirty = false;
    }
}

void UIElement::markTransformDirty() {
    m_isTransformDirty = true;
}
