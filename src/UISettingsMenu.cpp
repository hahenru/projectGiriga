#include <UI/UISettingsMenu.h>
#include <GLFW/glfw3.h>
#include <algorithm>

UISettingsMenu::UISettingsMenu(float x, float y, float width, float height,
                               IResourceManager& resources, EventBus& eventBus)
: UIPanel(x, y, width, height),
m_resources(resources),
m_eventBus(eventBus) {

    setName("SettingsMenu");
    setVisible(false);

    setBackgroundColor(0.08f, 0.08f, 0.12f, 0.95f);
    setBorderColor(0.35f, 0.40f, 0.50f, 1.0f);
    setBorderWidth(2.0f);
    setPadding(16.0f);

    float curW = getLocalWidth();
    float curH = getLocalHeight();

    // 1. 頂部標題與關閉按鈕
    m_titleText = emplaceChild<UIText>(getPaddingLeft(), getPaddingTop(), m_resources, U"系統偏好設定", 22);
    m_titleText->setColor(0.9f, 0.9f, 0.95f, 1.0f);

    float btnCloseSize = 24.0f;
    m_closeBtn = emplaceChild<UIButton>(curW - getPaddingRight() - btnCloseSize, getPaddingTop(), btnCloseSize, btnCloseSize);
    m_closeBtn->setLabel(m_resources, U"×", 0.9f, 0.9f, 0.9f, 1.0f);
    m_closeBtn->setOnClick([this]() { close(); });

    // 2. 左側欄位
    m_leftSidebar = emplaceChild<UIPanel>(0.0f, 0.0f, 0.0f, 0.0f);
    m_leftSidebar->setBackgroundColor(0.05f, 0.05f, 0.08f, 0.6f);
    m_leftSidebar->setBorderColor(0.25f, 0.25f, 0.30f, 1.0f);
    m_leftSidebar->setBorderWidth(1.0f);
    m_leftSidebar->setPadding(8.0f);

    // 直向排版容器：支援元件填滿寬度 (FitWidth)[cite: 7]
    m_tabLayout = m_leftSidebar->emplaceChild<UIVerticalLayout>(0.0f, 0.0f, 0.0f, 0.0f);
    m_tabLayout->setSpacing(6.0f);
    m_tabLayout->setFitWidth(true);

    // 3. 右側展示區 (掛載各分頁的選項內容)
    m_rightContentArea = emplaceChild<UIPanel>(0.0f, 0.0f, 0.0f, 0.0f);
    m_rightContentArea->setBackgroundColor(0.06f, 0.06f, 0.09f, 0.7f);
    m_rightContentArea->setBorderColor(0.25f, 0.25f, 0.30f, 1.0f);
    m_rightContentArea->setBorderWidth(1.0f);
    m_rightContentArea->setPadding(16.0f);

    // 4. 底部控制按鈕
    m_applyBtn = emplaceChild<UIButton>(0.0f, 0.0f, 100.0f, 34.0f);
    m_applyBtn->setLabel(m_resources, U"套用");
    m_applyBtn->setOnClick([this]() { close(); });

    m_cancelBtn = emplaceChild<UIButton>(0.0f, 0.0f, 100.0f, 34.0f);
    m_cancelBtn->setLabel(m_resources, U"關閉");
    m_cancelBtn->setOnClick([this]() { close(); });

    layoutChildren();
}

void UISettingsMenu::registerTab(const std::string& group, const std::u32string& groupTitle, int groupOrder,
                                 const std::string& tabId, const std::u32string& tabTitle, int tabOrder,
                                 PageBuilder builder) {
    auto it = std::find_if(m_registeredTabs.begin(), m_registeredTabs.end(),
                           [&tabId](const TabEntry& e) { return e.tabId == tabId; });
    if (it != m_registeredTabs.end()) return;

    m_registeredTabs.push_back({
        group, groupTitle, groupOrder,
        tabId, tabTitle, tabOrder,
        std::move(builder),
                               nullptr, nullptr
    });

    // 依照群組排序 -> 再依分頁內部排序
    std::sort(m_registeredTabs.begin(), m_registeredTabs.end(),
              [](const TabEntry& a, const TabEntry& b) {
                  if (a.groupOrder != b.groupOrder) {
                      return a.groupOrder < b.groupOrder;
                  }
                  return a.tabOrder < b.tabOrder;
              });

    rebuildTabsUI();
                                 }

                                 void UISettingsMenu::rebuildTabsUI() {
                                     m_tabLayout->clear();
                                     m_rightContentArea->clear();

                                     std::string currentGroup = "";

                                     for (auto& entry : m_registeredTabs) {
                                         // ★ 當進入新分組時，先在按鈕上方建立一個「分組標籤」
                                         if (entry.group != currentGroup) {
                                             currentGroup = entry.group;

                                             // 如果不是第一個分組，在群組上方留點空隙 (可以用較高的文字或 Spacer)
                                             auto* groupLabel = m_tabLayout->emplaceChild<UIText>(
                                                 0.0f, 0.0f, m_resources, entry.groupTitle, 14
                                             );
                                             // 分組標籤顏色偏灰藍色，作為微弱提示
                                             groupLabel->setColor(0.55f, 0.65f, 0.75f, 1.0f);
                                         }

                                         // 1. 左側建立分頁按鈕
                                         entry.tabButton = m_tabLayout->emplaceChild<UIButton>(0.0f, 0.0f, 0.0f, 32.0f);
                                         entry.tabButton->setLabel(m_resources, entry.tabTitle);
                                         std::string tid = entry.tabId;
                                         entry.tabButton->setOnClick([this, tid]() { selectTab(tid); });

                                         // 2. 右側建立該分頁的內容承載面板
                                         entry.pagePanel = m_rightContentArea->emplaceChild<UIPanel>(0.0f, 0.0f, 0.0f, 0.0f);
                                         entry.pagePanel->setSizePercent(100.0f, 100.0f);
                                         entry.pagePanel->setBackgroundColor(0.0f, 0.0f, 0.0f, 0.0f);
                                         entry.pagePanel->setBorderWidth(0.0f);
                                         entry.pagePanel->setVisible(false);

                                         // 呼叫 builder 注入具體的選項 (如滑桿、開關)
                                         if (entry.builder) {
                                             entry.builder(entry.pagePanel, m_resources, m_eventBus);
                                         }
                                     }

                                     // 預設開啟第一個分頁
                                     if (!m_registeredTabs.empty()) {
                                         if (m_activeTabId.empty()) {
                                             selectTab(m_registeredTabs.front().tabId);
                                         } else {
                                             selectTab(m_activeTabId);
                                         }
                                     }
                                 }

                                 void UISettingsMenu::selectTab(const std::string& tabId) {
                                     m_activeTabId = tabId;

                                     for (auto& entry : m_registeredTabs) {
                                         bool isActive = (entry.tabId == tabId);
                                         if (entry.pagePanel) {
                                             entry.pagePanel->setVisible(isActive);
                                         }
                                         if (entry.tabButton) {
                                             if (isActive) {
                                                 entry.tabButton->setColors({
                                                     .normal  = {0.25f, 0.40f, 0.60f, 1.0f},
                                                     .hovered = {0.30f, 0.50f, 0.70f, 1.0f},
                                                     .pressed = {0.20f, 0.35f, 0.55f, 1.0f}
                                                 });
                                             } else {
                                                 entry.tabButton->setColors({
                                                     .normal  = {0.14f, 0.15f, 0.18f, 1.0f},
                                                     .hovered = {0.20f, 0.22f, 0.26f, 1.0f},
                                                     .pressed = {0.10f, 0.12f, 0.14f, 1.0f}
                                                 });
                                             }
                                         }
                                     }
                                 }

                                 void UISettingsMenu::setSize(float width, float height) {
                                     UIPanel::setSize(width, height);
                                     layoutChildren();
                                 }

                                 void UISettingsMenu::layoutChildren() {
                                     float curW = getLocalWidth();
                                     float curH = getLocalHeight();

                                     if (m_titleText) m_titleText->setPosition(getPaddingLeft(), getPaddingTop());

                                     float btnCloseSize = 24.0f;
                                     if (m_closeBtn) {
                                         m_closeBtn->setPosition(curW - getPaddingRight() - btnCloseSize, getPaddingTop());
                                     }

                                     float contentTop = getPaddingTop() + 45.0f;
                                     float contentHeight = curH - contentTop - getPaddingBottom() - 45.0f;
                                     float contentWidth = getContentWidth();

                                     // 側邊欄佔 25% 寬
                                     float sidebarWidth = contentWidth * 0.25f;
                                     if (m_leftSidebar) {
                                         m_leftSidebar->setPosition(getPaddingLeft(), contentTop);
                                         m_leftSidebar->setSize(sidebarWidth, contentHeight);
                                     }
                                     if (m_tabLayout) {
                                         m_tabLayout->setPosition(m_leftSidebar->getPaddingLeft(), m_leftSidebar->getPaddingTop());
                                         m_tabLayout->setSize(m_leftSidebar->getContentWidth(), m_leftSidebar->getContentHeight());
                                     }

                                     // 右側選項區佔剩餘寬度
                                     float rightLeft = getPaddingLeft() + sidebarWidth + 12.0f;
                                     float rightWidth = contentWidth - sidebarWidth - 12.0f;
                                     if (m_rightContentArea) {
                                         m_rightContentArea->setPosition(rightLeft, contentTop);
                                         m_rightContentArea->setSize(rightWidth, contentHeight);
                                     }

                                     // 底部按鈕
                                     float btnW = 100.0f;
                                     float btnH = 34.0f;
                                     float bottomBtnY = curH - getPaddingBottom() - btnH;

                                     if (m_applyBtn) {
                                         m_applyBtn->setPosition(curW - getPaddingRight() - (btnW * 2.0f + 12.0f), bottomBtnY);
                                         m_applyBtn->setSize(btnW, btnH);
                                     }
                                     if (m_cancelBtn) {
                                         m_cancelBtn->setPosition(curW - getPaddingRight() - btnW, bottomBtnY);
                                         m_cancelBtn->setSize(btnW, btnH);
                                     }
                                 }

                                 void UISettingsMenu::open() { setVisible(true); }
                                 void UISettingsMenu::close() { setVisible(false); }

                                 bool UISettingsMenu::handleKey(int key, int scancode, int action, int mods) {
                                     if (!isVisible()) return false;
                                     if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                                         close();
                                         return true;
                                     }
                                     return UIPanel::handleKey(key, scancode, action, mods);
                                 }
