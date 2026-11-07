#ifndef GIRIGARA_UI_UI_SETTINGS_MENU_H
#define GIRIGARA_UI_UI_SETTINGS_MENU_H

#include <UI/UIPanel.h>
#include <UI/UIButton.h>
#include <UI/UIText.h>
#include <UI/UIVerticalLayout.h>
#include <Resource/ResourceManager.h>
#include <Event/EventBus.h>
#include <vector>
#include <string>
#include <functional>
#include <map>

class UISettingsMenu : public UIPanel {
public:
    using PageBuilder = std::function<void(UIPanel* contentArea, ResourceManager& resources, EventBus& eventBus)>;

    struct TabEntry {
        std::string group;          // 所屬分組 ID (例如 "system", "gameplay")
        std::u32string groupTitle;  // 分組上方顯示的標籤 (例如 U"系統設定")
        int groupOrder = 0;         // 分組本身的排序權重

        std::string tabId;          // 分頁 ID (例如 "audio")
        std::u32string tabTitle;    // 分頁按鈕文字 (例如 U"聲音")
        int tabOrder = 0;           // 分頁在該群組內的排序權重

        PageBuilder builder;

        // 運行期節點快取
        UIButton* tabButton{nullptr};
        UIPanel*  pagePanel{nullptr};
    };

    UISettingsMenu(float x, float y, float width, float height,
                   ResourceManager& resources, EventBus& eventBus);
    ~UISettingsMenu() override = default;

    // ★ 支援分組標籤的動態註冊介面
    void registerTab(const std::string& group, const std::u32string& groupTitle, int groupOrder,
                     const std::string& tabId, const std::u32string& tabTitle, int tabOrder,
                     PageBuilder builder);

    void open();
    void close();
    void selectTab(const std::string& tabId);

    void setSize(float width, float height);
    void layoutChildren();

    bool handleKey(int key, int scancode, int action, int mods) override;

private:
    void rebuildTabsUI();

    ResourceManager& m_resources;
    EventBus&        m_eventBus;

    std::vector<TabEntry> m_registeredTabs;
    std::string           m_activeTabId;

    // 外殼元件
    UIText*           m_titleText{nullptr};
    UIButton*         m_closeBtn{nullptr};
    UIPanel*          m_leftSidebar{nullptr};
    UIVerticalLayout* m_tabLayout{nullptr};
    UIPanel*          m_rightContentArea{nullptr};
    UIButton*         m_applyBtn{nullptr};
    UIButton*         m_cancelBtn{nullptr};
};

#endif // GIRIGARA_UI_UI_SETTINGS_MENU_H
