#include <UI/UIButton.h>
#include <UI/UIInputField.h>
#include <UI/UIText.h>
#include <UI/UIManager.h>
#include <UI/UIWindow.h>
#include <Event/Event.h>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
}

namespace {
    constexpr const char* UI_MANAGER_REGISTRY_KEY = "GIRIGARA_UI_MANAGER_INSTANCE";

    UIManager* getUIManagerFromLua(lua_State* L) {
        lua_pushstring(L, UI_MANAGER_REGISTRY_KEY);
        lua_gettable(L, LUA_REGISTRYINDEX);
        auto* mgr = static_cast<UIManager*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return mgr;
    }

    // 遞迴搜尋 UIContainer 樹
    UIElement* findInTreeByName(UIContainer& root, const std::string& name) {
        if (root.getName() == name) return &root;
        for (const auto& child : root.getChildren()) {
            if (!child) continue;
            if (child->getName() == name) return child.get();
            if (auto* container = dynamic_cast<UIContainer*>(child.get())) {
                if (auto* found = findInTreeByName(*container, name)) {
                    return found;
                }
            }
        }
        return nullptr;
    }

    UIElement* findInTreeByID(UIContainer& root, uint64_t id) {
        if (root.getID().get() == id) return &root;
        for (const auto& child : root.getChildren()) {
            if (!child) continue;
            if (child->getID().get() == id) return child.get();
            if (auto* container = dynamic_cast<UIContainer*>(child.get())) {
                if (auto* found = findInTreeByID(*container, id)) {
                    return found;
                }
            }
        }
        return nullptr;
    }

    // @luaFunc BindUIUpdate(element: lightuserdata, callback: function)
    int lua_BindUIUpdate(lua_State* L) {
        auto* element = static_cast<UIElement*>(lua_touserdata(L, 1));
        if (!element || !lua_isfunction(L, 2)) {
            return luaL_error(L, "Invalid arguments to BindUIUpdate: expected (element, function)");
        }
        lua_pushvalue(L, 2);
        element->setScriptUpdate(LuaRef::fromStack(L));
        return 0;
    }

    // @luaFunc UI_SetPosition(element: lightuserdata, x: number, y: number)
    int lua_UI_SetPosition(lua_State* L) {
        auto* element = static_cast<UIElement*>(lua_touserdata(L, 1));
        float x = static_cast<float>(luaL_checknumber(L, 2));
        float y = static_cast<float>(luaL_checknumber(L, 3));
        if (element) {
            element->setPosition(x, y);
        }
        return 0;
    }

    // @luaFunc FindUI(name: string) -> lightuserdata|nil
    int lua_FindUI(lua_State* L) {
        const char* name = luaL_checkstring(L, 1);
        UIManager* mgr = getUIManagerFromLua(L);
        UIElement* elem = (mgr && name) ? mgr->findElementByName(name) : nullptr;
        if (elem) {
            lua_pushlightuserdata(L, elem);
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    // @luaFunc GetUIByUUID(id: integer) -> lightuserdata|nil
    int lua_GetUIByUUID(lua_State* L) {
        uint64_t id = static_cast<uint64_t>(luaL_checkinteger(L, 1));
        UIManager* mgr = getUIManagerFromLua(L);
        UIElement* elem = mgr ? mgr->findElementByID(id) : nullptr;
        if (elem) {
            lua_pushlightuserdata(L, elem);
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    // @luaFunc UI_GetUUID(element: lightuserdata) -> integer|nil
    int lua_UI_GetUUID(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        if (elem) {
            lua_pushinteger(L, static_cast<lua_Integer>(elem->getID().get()));
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    // @luaFunc UI_GetName(element: lightuserdata) -> string|nil
    int lua_UI_GetName(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        if (elem) {
            lua_pushstring(L, elem->getName().c_str());
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    // @luaFunc UI_SetVisible(element: lightuserdata, visible: boolean)
    int lua_UI_SetVisible(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        bool visible = lua_toboolean(L, 2) != 0;
        if (elem) {
            elem->setVisible(visible);
        }
        return 0;
    }

    // @luaFunc UI_GetX(elem: lightuserdata) -> number
    int lua_UI_GetX(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        lua_pushnumber(L, static_cast<lua_Number>(elem ? elem->getX() : 0.0f));
        return 1;
    }

    // @luaFunc UI_GetY(elem: lightuserdata) -> number
    int lua_UI_GetY(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        lua_pushnumber(L, static_cast<lua_Number>(elem ? elem->getY() : 0.0f));
        return 1;
    }

    // @luaFunc UI_IsDragging(elem: lightuserdata) -> boolean
    int lua_UI_IsDragging(lua_State* L) {
        auto* win = dynamic_cast<UIWindow*>(static_cast<UIElement*>(lua_touserdata(L, 1)));
        lua_pushboolean(L, (win && win->isDragging()) ? 1 : 0);
        return 1;
    }

    // @luaFunc SetPositionPercent(elem: lightuserdata, xPercent: number, yPercent: number)
    int lua_UI_SetPositionPercent(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        float x = static_cast<float>(luaL_checknumber(L, 2));
        float y = static_cast<float>(luaL_checknumber(L, 3));
        if (elem) {
            elem->setPositionPercent(x, y);
        }
        return 0;
    }

    // @luaFunc UI_OnClick(button: lightuserdata, callback: function)
    int lua_UI_OnClick(lua_State* L) {
        auto* btn = dynamic_cast<UIButton*>(static_cast<UIElement*>(lua_touserdata(L, 1)));
        if (!btn || !lua_isfunction(L, 2)) {
            return luaL_error(L, "UI_OnClick expected (UIButton, function)");
        }
        lua_pushvalue(L, 2);
        auto cbRef = std::make_shared<LuaRef>(LuaRef::fromStack(L));
        btn->setOnClick([L, cbRef]() {
            if (!cbRef->isValid()) return;
            cbRef->push();
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                lua_pop(L, 1);
            }
        });
        return 0;
    }

    // @luaFunc UI_GetText(elem: lightuserdata) -> string
    int lua_UI_GetText(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        if (auto* input = dynamic_cast<UIInputField*>(elem)) {
            lua_pushstring(L, input->getText().c_str());
        } else if (auto* txt = dynamic_cast<UIText*>(elem)) {
            lua_pushstring(L, EncodingUtils::utf32ToUtf8(txt->getText()).c_str());
        } else if (auto* btn = dynamic_cast<UIButton*>(elem); btn && btn->getLabel()) {
            lua_pushstring(L, EncodingUtils::utf32ToUtf8(btn->getLabel()->getText()).c_str());
        } else {
            lua_pushstring(L, "");
        }
        return 1;
    }

    // @luaFunc UI_SetText(elem: lightuserdata, str: string)
    int lua_UI_SetText(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        const char* str = luaL_checkstring(L, 2);
        UIManager* mgr = getUIManagerFromLua(L);
        if (!elem || !str) return 0;

        if (auto* input = dynamic_cast<UIInputField*>(elem)) {
            input->setText(str);
        } else if (auto* txt = dynamic_cast<UIText*>(elem)) {
            txt->setText(EncodingUtils::utf8ToUtf32(str));
        } else if (auto* btn = dynamic_cast<UIButton*>(elem); btn && mgr) {
            btn->setLabel(mgr->getResources(), EncodingUtils::utf8ToUtf32(str));
        }
        return 0;
    }

    // @luaFunc UI_SetButtonLocked(button: lightuserdata, locked: boolean)
    int lua_UI_SetButtonLocked(lua_State* L) {
        auto* btn = dynamic_cast<UIButton*>(static_cast<UIElement*>(lua_touserdata(L, 1)));
        bool locked = lua_toboolean(L, 2) != 0;
        if (btn) {
            btn->setLocked(locked);
        }
        return 0;
    }

    // @luaFunc UI_SetFontSize(elem: lightuserdata, fontSize: integer)
    int lua_UI_SetFontSize(lua_State* L) {
        auto* elem = static_cast<UIElement*>(lua_touserdata(L, 1));
        unsigned int sz = static_cast<unsigned int>(luaL_checkinteger(L, 2));
        if (auto* txt = dynamic_cast<UIText*>(elem)) {
            txt->setFontSize(sz);
        } else if (auto* btn = dynamic_cast<UIButton*>(elem); btn && btn->getLabel()) {
            btn->getLabel()->setFontSize(sz);
        } else if (auto* input = dynamic_cast<UIInputField*>(elem)) {
            input->setFontSize(sz);
        }
        return 0;
    }

    // @luaFunc UI_GetScreenSize() -> number, number
    int lua_UI_GetScreenSize(lua_State* L) {
        UIManager* mgr = getUIManagerFromLua(L);
        if (mgr) {
            lua_pushnumber(L, static_cast<lua_Number>(mgr->getRootUI().getWidth()));
            lua_pushnumber(L, static_cast<lua_Number>(mgr->getRootUI().getHeight()));
        } else {
            lua_pushnumber(L, 960.0);
            lua_pushnumber(L, 640.0);
        }
        return 2;
    }

    UIPropMap parsePropMapFromLuaTable(lua_State* L, int tableIdx) {
        UIPropMap props;
        if (!lua_istable(L, tableIdx)) return props;

        lua_pushnil(L);
        while (lua_next(L, tableIdx) != 0) {
            if (lua_type(L, -2) == LUA_TSTRING) {
                std::string key = lua_tostring(L, -2);
                if (lua_isboolean(L, -1)) {
                    props[key] = (lua_toboolean(L, -1) != 0);
                } else if (lua_isinteger(L, -1)) {
                    props[key] = static_cast<int>(lua_tointeger(L, -1));
                } else if (lua_isnumber(L, -1)) {
                    props[key] = static_cast<float>(lua_tonumber(L, -1));
                } else if (lua_isstring(L, -1)) {
                    props[key] = std::string(lua_tostring(L, -1));
                }
            }
            lua_pop(L, 1);
        }
        return props;
    }

    // @luaFunc CreateUI(typeName: string, props?: table, parent?: lightuserdata) -> lightuserdata|nil
    int lua_CreateUI(lua_State* L) {
        const char* typeName = luaL_checkstring(L, 1);
        UIPropMap props = (lua_gettop(L) >= 2) ? parsePropMapFromLuaTable(L, 2) : UIPropMap{};
        auto* parent = (lua_gettop(L) >= 3) ? dynamic_cast<UIContainer*>(static_cast<UIElement*>(lua_touserdata(L, 3))) : nullptr;

        UIManager* mgr = getUIManagerFromLua(L);
        UIElement* elem = (mgr && typeName) ? mgr->createElement(typeName, props, parent) : nullptr;
        if (elem) {
            lua_pushlightuserdata(L, elem);
        } else {
            lua_pushnil(L);
        }
        return 1;
    }
}

UIManager::UIManager(Shader& uiShader,
                     EventBus& eventBus,
                     KeyboardSystem& keyboard,
                     ConsoleSystem& console,
                     LoggerSystem& logger,
                     IResourceManager& resources,
                     float screenWidth, float screenHeight)
                         : m_renderer(uiShader, screenWidth, screenHeight),
                           m_eventBus(eventBus),
                           m_resources(resources),
                           m_screenWidth(screenWidth),
                           m_screenHeight(screenHeight) {
    m_rootUI.setPosition(0.0f, 0.0f);
    m_rootUI.setSize(screenWidth, screenHeight);
    m_rootUI.setPassThrough(true);

#ifndef GIRIGARA_TOOL
    // 1. 掛載設定選單 (先於 Console，確保 Console 絕對置頂)
    m_settingsMenu = m_rootUI.emplaceChild<UISettingsMenu>(
        0.0f, 0.0f,
        screenWidth, screenHeight,
        m_resources, m_eventBus
    );

    // 2. 掛載 Console
    m_console = m_rootUI.emplaceChild<UIConsole>(
        0.0f, 0.0f, screenWidth, screenHeight,
        m_eventBus, console, logger, m_resources
    );
    m_console->setName("InGameConsole");
    m_console->setVisible(false);

    // 3. 註冊波浪鍵呼叫 Console
    keyboard.registerShortcut(
        GLFW_KEY_GRAVE_ACCENT, 0,
        [this]() { toggleConsole(); }
    );

    // 4. ★ 註冊 ESC 呼叫設定選單 (由 UIManager 統一管理層級優先權)
    keyboard.registerShortcut(
        GLFW_KEY_ESCAPE, 0,
        [this]() { toggleSettingsMenu(); }
    );
#else
	(void)keyboard;
    (void)console;
    (void)logger;
#endif

    bindEvents();
    registerScriptBindings();
    m_factory.registerBuiltinTypes();
}

UIManager::~UIManager() {
    m_eventBus.unsubscribeByOwner(getID());
    unregisterScriptBindings();
}

void UIManager::toggleSettingsMenu() {
#ifndef GIRIGARA_TOOL
    if (!m_settingsMenu) return;

    // ★ 防禦 1：如果 Console 正開著，ESC 的優先權是關閉 Console，不觸發設定選單
    if (m_console && m_console->isVisible()) {
        toggleConsole();
        return;
    }

    // ★ 防禦 2：正常切換設定選單開關
    bool next = !m_settingsMenu->isVisible();
    m_settingsMenu->setVisible(next);

    if (next) {
        m_rootUI.setFocusedElement(m_settingsMenu);
    } else {
        if (m_rootUI.getFocusedElement() == m_settingsMenu) {
            m_rootUI.setFocusedElement(nullptr);
        }
    }
#endif
}

void UIManager::onWindowResize(float width, float height) {
    m_screenWidth = width;
    m_screenHeight = height;
    m_renderer.setScreenSize(width, height);
    m_rootUI.setSize(width, height);

	#ifndef GIRIGARA_TOOL
    if (m_console) {
        m_console->setSize(width, height);
    }
	#endif

    if (m_activeSceneUI) {
        m_activeSceneUI->setSize(width, height);
    }
	#ifndef GIRIGARA_TOOL
    if (m_settingsMenu) {
        m_settingsMenu->setPosition(0.0f, 0.0f);
        m_settingsMenu->setSize(width, height);
    }
	#endif
}

void UIManager::bindEvents() {
    // A. 場景 UI 移交事件
    m_eventBus.subscribe<SceneUIChangedEvent>([this](SceneUIChangedEvent& e) {
        if (m_activeSceneUI) {
            m_dyingSceneUI.push_back({ std::move(m_activeSceneUI), 0.0f, 0.25f });
        }

        m_activeSceneUI = std::move(e.uiTree);
        if (m_activeSceneUI) {
            m_activeSceneUI->setPosition(0.0f, 0.0f);
            m_activeSceneUI->setSize(m_screenWidth, m_screenHeight);
            m_activeSceneUI->setPassThrough(true); // 場景根容器也是穿透的
        }
    }, getID());

    // B. 視窗縮放事件
    m_eventBus.subscribe<WindowResizeEvent>([this](WindowResizeEvent& e) {
        onWindowResize(static_cast<float>(e.width), static_cast<float>(e.height));
    }, getID());

    // C. ★ 關鍵修正：滑鼠移動事件派發 (用於按鈕 Hover 更新)
    m_eventBus.subscribe<MouseMovedEvent>([this](MouseMovedEvent& e) {
        // 先由頂層全域 UI 處理 (若 Console 打開且滑鼠在其上，會攔截)
        bool handled = m_rootUI.handleMouseMove(e.x, e.y);

        // 若全域 UI 沒攔截 (例如 Console 隱藏或滑鼠在 Console 外)，再交給場景 UI
        if (!handled && m_activeSceneUI && m_activeSceneUI->isVisible()) {
            m_activeSceneUI->handleMouseMove(e.x, e.y);
        }
    }, getID());

    // D. ★ 關鍵修正：滑鼠點擊事件派發 (用於按鈕 Click 與輸入框 Focus)
    m_eventBus.subscribePriority<MouseButtonEvent>([this](MouseButtonEvent& e) {
        // 1. 優先送給頂層全域樹 (Console)
        if (m_rootUI.handleMouseButton(e.button, e.action, e.mods, e.x, e.y)) {
            e.isHandled = true; // 被 Console 攔截，底層按鈕絕不觸發
            return;
        }

        // 2. 全域樹未攔截，送給活躍場景 UI 樹
        if (m_activeSceneUI && m_activeSceneUI->isVisible()) {
            if (m_activeSceneUI->handleMouseButton(e.button, e.action, e.mods, e.x, e.y)) {
                e.isHandled = true;
            }
        }
    }, getID());
}

void UIManager::toggleConsole() {
#ifndef GIRIGARA_TOOL
    if (!m_console) return;
    bool next = !m_console->isVisible();
    m_console->setVisible(next);

    if (next) {
        m_rootUI.setFocusedElement(m_console);
        if (auto* input = m_console->getInputField()) {
            input->setFocused(true);
        }
    } else {
        // ★ 關閉時徹底清除焦點，防止幽靈焦點殘留
        m_rootUI.setFocusedElement(nullptr);
        if (auto* input = m_console->getInputField()) {
            input->setFocused(false);
        }
    }
#endif
}


void UIManager::update(float dt) {
    m_rootUI.update(dt);

    if (m_activeSceneUI) {
        m_activeSceneUI->update(dt);
    }

    for (auto it = m_dyingSceneUI.begin(); it != m_dyingSceneUI.end();) {
        it->currentLifetime += dt;
        it->tree->update(dt);
        if (it->currentLifetime >= it->maxLifetime) {
            it = m_dyingSceneUI.erase(it);
        } else {
            ++it;
        }
    }
}

void UIManager::render() {
    m_renderer.begin();

    // 1. 先畫即將消亡的場景 UI
    for (auto& entry : m_dyingSceneUI) {
        entry.tree->render(m_renderer);
    }

    // 2. 再畫當前活躍場景 UI
    if (m_activeSceneUI && m_activeSceneUI->isVisible()) {
        m_activeSceneUI->render(m_renderer);
    }

    // 3. 最後畫全域層 (Console / Toast 置頂)
    m_rootUI.render(m_renderer);

    m_renderer.end();
}

void UIManager::registerScriptBindings() {
    m_eventBus.publish(RegisterScriptBindingEvent{
        getID(),
                       UI_MANAGER_REGISTRY_KEY,
                       this,
                       {
                           { "BindUIUpdate",       lua_BindUIUpdate },
                           { "UI_SetPosition",     lua_UI_SetPosition },
                           { "FindUI",             lua_FindUI },
                           { "GetUIByUUID",        lua_GetUIByUUID },
                           { "UI_GetUUID",         lua_UI_GetUUID },
                           { "UI_GetName",         lua_UI_GetName },
                           { "UI_SetVisible",      lua_UI_SetVisible },
                           { "UI_GetX",            lua_UI_GetX },
                           { "UI_GetY",            lua_UI_GetY },
                           { "UI_IsDragging",      lua_UI_IsDragging },
                           { "SetPositionPercent", lua_UI_SetPositionPercent },
                           { "CreateUI", lua_CreateUI },
                           { "UI_OnClick",         lua_UI_OnClick },
                           { "UI_GetText",         lua_UI_GetText },
                           { "UI_SetText",         lua_UI_SetText },
                           { "UI_SetButtonLocked", lua_UI_SetButtonLocked },
                           { "UI_SetFontSize",     lua_UI_SetFontSize },
                           { "UI_GetScreenSize",   lua_UI_GetScreenSize }
                       }
    });
}

void UIManager::unregisterScriptBindings() {
    m_eventBus.publish(UnregisterScriptBindingEvent{
        getID()
    });
}

UIElement* UIManager::findElementByName(const std::string& name) {
    if (m_activeSceneUI) {
        if (auto* found = findInTreeByName(*m_activeSceneUI, name)) return found;
    }
    return findInTreeByName(m_rootUI, name);
}

UIElement* UIManager::findElementByID(uint64_t id) {
    if (m_activeSceneUI) {
        if (auto* found = findInTreeByID(*m_activeSceneUI, id)) return found;
    }
    return findInTreeByID(m_rootUI, id);
}

UIElement* UIManager::createElement(const std::string& typeName, const UIPropMap& props, UIContainer* parent) {
    UIBuildContext ctx{ m_resources, m_eventBus };
    auto elem = m_factory.create(typeName, ctx, props);
    if (!elem) return nullptr;

    if (auto name = UIFactory::getString(props, "name"); !name.empty()) {
        elem->setName(name);
    }

    // ★ 新增：支援百分比座標與尺寸（例如 xPercent = 5.0, wPercent = 90.0）
    if (props.contains("xPercent") || props.contains("yPercent")) {
        elem->setPositionPercent(
            UIFactory::getFloat(props, "xPercent", 0.0f),
                                 UIFactory::getFloat(props, "yPercent", 0.0f)
        );
    }
    if (props.contains("wPercent") || props.contains("hPercent")) {
        elem->setSizePercent(
            UIFactory::getFloat(props, "wPercent", 100.0f),
                             UIFactory::getFloat(props, "hPercent", 100.0f)
        );
    }

    UIElement* rawPtr = elem.get();
    UIContainer* target = parent ? parent : (m_activeSceneUI ? m_activeSceneUI.get() : &m_rootUI);
    target->addChild(std::move(elem));
    return rawPtr;
}


