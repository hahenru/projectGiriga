#include <System/SettingsRegistry.h>
#include <UI/UIButton.h>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
}

namespace {
    constexpr const char* SETTINGS_REGISTRY_KEY = "GIRIGARA_SETTINGS_INSTANCE";

    SettingsRegistry* getSettingsFromLua(lua_State* L) {
        lua_pushstring(L, SETTINGS_REGISTRY_KEY);
        lua_gettable(L, LUA_REGISTRYINDEX);
        auto* settings = static_cast<SettingsRegistry*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return settings;
    }

    SettingValue parseSettingValueFromLua(lua_State* L, int idx) {
        if (lua_isboolean(L, idx)) {
            return (lua_toboolean(L, idx) != 0);
        } else if (lua_isinteger(L, idx)) {
            return static_cast<int>(lua_tointeger(L, idx));
        } else if (lua_isnumber(L, idx)) {
            return static_cast<float>(lua_tonumber(L, idx));
        } else if (lua_isstring(L, idx)) {
            return std::string(lua_tostring(L, idx));
        }
        return false;
    }

    // ChangeSettingByName("Audio.Volume.BGM", 0.8)
    int lua_ChangeSettingByName(lua_State* L) {
        SettingsRegistry* settings = getSettingsFromLua(L);
        if (!settings) return 0;

        const char* path = luaL_checkstring(L, 1);
        SettingValue val = parseSettingValueFromLua(L, 2);

        bool success = settings->setByName(path, val, true);
        lua_pushboolean(L, success ? 1 : 0);
        return 1;
    }

    // ChangeSettingByUUID(uuid, 0.8)
    int lua_ChangeSettingByUUID(lua_State* L) {
        SettingsRegistry* settings = getSettingsFromLua(L);
        if (!settings) return 0;

        uint64_t uuid = static_cast<uint64_t>(luaL_checkinteger(L, 1));
        SettingValue val = parseSettingValueFromLua(L, 2);

        bool success = settings->setByUUID(uuid, val, true);
        lua_pushboolean(L, success ? 1 : 0);
        return 1;
    }

    // SetSettingEnabled(uuid, enabled)
    int lua_SetSettingEnabled(lua_State* L) {
        SettingsRegistry* settings = getSettingsFromLua(L);
        if (!settings) return 0;

        uint64_t uuid = static_cast<uint64_t>(luaL_checkinteger(L, 1));
        bool enabled = (lua_toboolean(L, 2) != 0);

        settings->setEnabled(uuid, enabled);
        return 0;
    }
}

SettingsRegistry::SettingsRegistry(EventBus& eventBus, LoggerSystem& logger)
: m_eventBus(eventBus),
m_logger(logger) {
    registerScriptBindings();
    bindEvents();
}

SettingsRegistry::~SettingsRegistry() {
    m_eventBus.unsubscribeByOwner(getID());
    unregisterScriptBindings();
}

void SettingsRegistry::registerScriptBindings() {
    m_eventBus.publish(RegisterScriptBindingEvent{
        getID(),
        SETTINGS_REGISTRY_KEY,
        this,
        {
            { "ChangeSettingByName", lua_ChangeSettingByName },
            { "ChangeSettingByUUID", lua_ChangeSettingByUUID },
            { "SetSettingEnabled",   lua_SetSettingEnabled }
        }
    });
}

void SettingsRegistry::unregisterScriptBindings() {
    m_eventBus.publish(UnregisterScriptBindingEvent{
        getID()
    });
}

void SettingsRegistry::bindEvents() {
    // 監聽遠端設定修改請求（支援路徑與 UUID 兩種模式）
    m_eventBus.subscribe<RemoteSettingRequestEvent>([this](RemoteSettingRequestEvent& e) {
        if (!e.path.empty()) {
            e.succeeded = setByName(e.path, e.value, e.force);
        } else if (e.uuid != 0) {
            e.succeeded = setByUUID(e.uuid, e.value, e.force);
        }
    }, getID());
}

uint64_t SettingsRegistry::registerSetting(const std::string& className,
    const std::string& groupName,
    const std::string& keyName,
    std::function<void(const SettingValue&)> setter,
    std::function<SettingValue()> getter,
    UIElement* uiElement) {
        SettingItem item;
        item.className = className;
        item.groupName = groupName;
        item.keyName = keyName;
        item.fullPath = className + "." + groupName + "." + keyName;
        item.setter = std::move(setter);
        item.getter = std::move(getter);
        item.boundUIElement = uiElement;

        uint64_t rawId = item.id.get();

        m_itemsByPath[item.fullPath] = item;
        m_pathToUUID[item.fullPath] = rawId;
        m_uuidToPath[rawId] = item.fullPath;

        m_logger.info("[Settings] 已註冊設定項目: " + item.fullPath + " (ID: " + std::to_string(rawId) + ")");
        return rawId;
}

void SettingsRegistry::bindUIElement(uint64_t uuid, UIElement* elem) {
    if (auto* item = findByUUID(uuid)) {
        item->boundUIElement = elem;
    }
}

bool SettingsRegistry::setByName(const std::string& path, const SettingValue& val, bool force) {
    auto* item = findByName(path);
    if (!item) {
        m_logger.warn("[Settings] 找不到設定項目: " + path);
        return false;
    }

    if (!item->isEnabled && !force) {
        m_logger.warn("[Settings] " + path + " 目前處於停用狀態 (Disabled)");
        return false;
    }

    if (item->setter) {
        item->setter(val);
        m_logger.info("[Settings] " + path + " 數值已更新");

        // 廣播設定已變更事件，通知 UISettingsMenu 同步刷新畫面控制項
        m_eventBus.publish(SettingChangedEvent{
            item->fullPath,
            item->id.get(),
            val
        });
    }
    return true;
}

bool SettingsRegistry::setByUUID(uint64_t uuid, const SettingValue& val, bool force) {
    auto* item = findByUUID(uuid);
    if (!item) {
        m_logger.warn("[Settings] 找不到 UUID 為 " + std::to_string(uuid) + " 的設定項目");
        return false;
    }
    return setByName(item->fullPath, val, force);
}

void SettingsRegistry::setEnabled(uint64_t uuid, bool enabled) {
    auto* item = findByUUID(uuid);
    if (!item) return;

    item->isEnabled = enabled;
    m_logger.info("[Settings] " + item->fullPath + " 狀態變更為: " + (enabled ? "啟用" : "停用"));

    if (item->boundUIElement) {
        if (auto* btn = dynamic_cast<UIButton*>(item->boundUIElement)) {
            btn->setLocked(!enabled);
        }
    }
}

SettingItem* SettingsRegistry::findByName(const std::string& path) {
    auto it = m_itemsByPath.find(path);
    return it != m_itemsByPath.end() ? &it->second : nullptr;
}

SettingItem* SettingsRegistry::findByUUID(uint64_t uuid) {
    auto it = m_uuidToPath.find(uuid);
    if (it != m_uuidToPath.end()) {
        return findByName(it->second);
    }
    return nullptr;
}
