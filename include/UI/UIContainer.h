#ifndef GIRIGARA_UI_UI_CONTAINER_H
#define GIRIGARA_UI_UI_CONTAINER_H

#include <UI/UIElement.h>
#include <vector>
#include <memory>

class UIRenderer;

class UIContainer : public UIElement {
public:
    UIContainer() = default;
    UIContainer(float x, float y) : UIElement(x, y, 0.0f, 0.0f) {}
    UIContainer(float x, float y, float width, float height)
    : UIElement(x, y, width, height) {}
    ~UIContainer() override = default;

    template <typename T, typename... Args>
    T* emplaceChild(Args&&... args) {
        auto child = std::make_unique<T>(std::forward<Args>(args)...);
        child->setParent(this); // ★ 補上這行，確保 parent 樹狀鏈條完整！
        T* ptr = child.get();
        m_children.push_back(std::move(child));
        return ptr;
    }

    void addChild(std::unique_ptr<UIElement> child);
    void clear();

    void setFocusedElement(UIElement* element);
    UIElement* getFocusedElement() const { return m_focusedElement; }

    // ★ 設定是否開啟穿透模式 (預設為 false，即非穿透實體容器)
    void setPassThrough(bool passThrough) { m_passThrough = passThrough; }
    bool isPassThrough() const { return m_passThrough; }

    // 核心補齊：遞迴更新子節點
    void update(float dt) override;
    void render(UIRenderer& renderer) override;
    bool handleMouseMove(double x, double y) override;
    bool handleMouseButton(int button, int action, int mods, double x, double y) override;
    bool handleKey(int key, int scancode, int action, int mods) override;
    // ★ 取得子節點列表，供視窗縮放時遍歷校正
    const std::vector<std::unique_ptr<UIElement>>& getChildren() const { return m_children; }
    void bringToFront(UIElement* element);
	
	// ★ 覆寫：容器自身位移時，通知所有子節點
    void markTransformDirty() override;

    // ★ 標記結構變更（子節點新增、刪除、順序調整）
    void markHierarchyDirty() { m_isHierarchyDirty = true; }
    bool isHierarchyDirty() const { return m_isHierarchyDirty; }
    void clearHierarchyDirty() { m_isHierarchyDirty = false; }

    void setClipToBounds(bool clip) { m_clipToBounds = clip; }

protected:
    std::vector<std::unique_ptr<UIElement>> m_children;

private:
    UIElement* m_focusedElement = nullptr;
    bool m_passThrough = false; // ★ 是否讓未被子元件處理的事件穿透
    bool m_clipToBounds = false;
	bool m_isHierarchyDirty = true; // ★ 節點樹結構是否改變
};

#endif // GIRIGARA_UI_UI_CONTAINER_H
