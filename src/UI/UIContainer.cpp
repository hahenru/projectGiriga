#include <UI/UIContainer.h>
#include <Renderer/UIRenderer.h>
#include <UI/UIWindow.h>
#include <algorithm>
#include <GLFW/glfw3.h>

void UIContainer::addChild(std::unique_ptr<UIElement> child) {
    if (child) {
        child->setParent(this);
        child->markTransformDirty(); // 剛加入時確保重新計算世界座標
        m_children.push_back(std::move(child));
        markHierarchyDirty();        // ★ 標記樹結構變更
    }
}

void UIContainer::clear() {
    m_focusedElement = nullptr;
    m_children.clear();
    markHierarchyDirty();            // ★ 標記樹結構變更
}

void UIContainer::setFocusedElement(UIElement* element) {
    if (m_focusedElement == element) return;

    if (m_focusedElement) {
        m_focusedElement->setFocused(false);
    }
    m_focusedElement = element;
    if (m_focusedElement) {
        m_focusedElement->setFocused(true);
    }
}

void UIContainer::update(float dt) {
    UIElement::update(dt);
    if (!isVisible()) return;

    for (auto& child : m_children) {
        if (child && child->isVisible()) {
            child->update(dt);
        }
    }
}

void UIContainer::render(UIRenderer& renderer) {
    if (!isVisible()) return;

    bool clip = m_clipToBounds && (getWidth() > 0.0f && getHeight() > 0.0f);
    if (clip) {
        renderer.pushClipRect(getGlobalX(), getGlobalY(), getLocalWidth(), getLocalHeight());
    }

    for (auto& child : m_children) {
        if (child && child->isVisible()) {
            child->render(renderer);
        }
    }

    if (clip) {
        renderer.popClipRect();
    }

    clearHierarchyDirty();
}


bool UIContainer::handleMouseMove(double x, double y) {
    if (!isVisible()) return false;

    // 1. 優先由子元件處理
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it) && (*it)->isVisible()) {
            if ((*it)->handleMouseMove(x, y)) {
                return true;
            }
        }
    }

    // 2. 若子元件沒處理：
    // 如果開啟了穿透模式，一律回傳 false 放行給底層場景
    if (m_passThrough) {
        return false;
    }

    // 若非穿透容器，則看滑鼠是否落在容器自身範圍內
    return contains(x, y);
}

bool UIContainer::handleMouseButton(int button, int action, int mods, double x, double y) {
    if (!isVisible()) return false;

    // 1. 優先由子元件處理 (例如點到 InGameConsole 裡的按鈕或文字框)
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it) && (*it)->isVisible()) {
            if ((*it)->handleMouseButton(button, action, mods, x, y)) {
                UIElement* clickedElement = it->get();
                setFocusedElement(clickedElement);

                // ★ 加在這裡：當滑鼠按下且子元件成功消耗事件時且該物件允許，將其提升至最頂層
                if (action == GLFW_PRESS && clickedElement->canBringToFront()) {
                    bringToFront(clickedElement);
                }

                return true; // 子元件消耗了事件
            }
        }
    }

    // 2. 若子元件均未處理：
    // 若為穿透容器 (如全域 UI 畫布)，絕不吃掉事件，讓點擊穿透到底下的場景
    if (m_passThrough) {
        return false;
    }

    // 若為非穿透容器 (如實體面板)，點在自己身上就攔截，不讓點擊穿透到身後的東西
    return contains(x, y);
}

bool UIContainer::handleKey(int key, int scancode, int action, int mods) {
    if (!isVisible()) return false;

    if (m_focusedElement && m_focusedElement->isVisible()) {
        if (m_focusedElement->handleKey(key, scancode, action, mods)) {
            return true;
        }
    }

    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it) && (*it)->isVisible() && it->get() != m_focusedElement) {
            if ((*it)->handleKey(key, scancode, action, mods)) {
                return true;
            }
        }
    }

    // 鍵盤未被任何子節點處理，穿透模式下放行
    return !m_passThrough;
}

void UIContainer::bringToFront(UIElement* element) {
    if (!element || m_children.empty()) return;
    auto it = std::find_if(m_children.begin(), m_children.end(),
                           [element](const auto& ptr) { return ptr.get() == element; });
    if (it != m_children.end() && it != m_children.end() - 1) {
        auto target = std::move(*it);
        m_children.erase(it);
        m_children.push_back(std::move(target));
        markHierarchyDirty();        // ★ 渲染順序改變，標記髒
    }
}

void UIContainer::markTransformDirty() {
    UIElement::markTransformDirty();
    for (auto& child : m_children) {
        if (child) {
            child->markTransformDirty(); // 遞迴標記所有子節點
        }
    }
}


