// include/System/SettingsRegistry.h
#ifndef GIRIGARA_SYSTEM_SETTINGS_REGISTRY_H
#define GIRIGARA_SYSTEM_SETTINGS_REGISTRY_H

#include <Core/UUID.h>
#include <System/LoggerSystem.h>
#include <UI/UIElement.h>
#include <UI/UIButton.h>
#include <string>
#include <unordered_map>
#include <functional>
#include <variant>

using SettingValue = std::variant<bool, int, float, std::string>;

struct SettingItem {
    Core::UUID id;
    std::string className;
    std::string groupName;
    std::string keyName;
    std::string fullPath;

    bool isEnabled = true;

    std::function<void(const SettingValue&)> setter;
    std::function<SettingValue()> getter;

    UIElement* boundUIElement = nullptr;
};

class SettingsRegistry {
public:
    // ★ 依賴注入 LoggerSystem
    explicit SettingsRegistry(LoggerSystem& logger)
    : m_logger(logger) {}

    ~SettingsRegistry() = default;

    SettingsRegistry(const SettingsRegistry&) = delete;
    SettingsRegistry& operator=(const SettingsRegistry&) = delete;

    uint64_t registerSetting(const std::string& className,
const std::string& groupName,
const std::string& keyName,
std::function<void(const SettingValue&)> setter,
std::function<SettingValue()> getter,
UIElement* uiElement = nullptr) {
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

void bindUIElement(uint64_t uuid, UIElement* elem) {
    if (auto* item = findByUUID(uuid)) {
        item->boundUIElement = elem;
    }
}

bool setByName(const std::string& path, const SettingValue& val, bool force = true) {
    auto* item = findByName(path);
    if (!item) {
        m_logger.warn("[Settings] 找不到設定項目: " + path);
        return false;
    }

    if (!item->isEnabled && !force) {
        // ★ 取代 std::cout，直接寫進 Logger，並自動反映至 UIConsole
        m_logger.warn("[Settings] " + path + " 目前處於停用狀態 (Disabled)");
        return false;
    }

    if (item->setter) {
        item->setter(val);
        m_logger.info("[Settings] " + path + " 數值已更新");
    }
    return true;
}

bool setByUUID(uint64_t uuid, const SettingValue& val, bool force = true) {
    auto* item = findByUUID(uuid);
    if (!item) {
        m_logger.warn("[Settings] 找不到 UUID 為 " + std::to_string(uuid) + " 的設定項目");
        return false;
    }
    return setByName(item->fullPath, val, force);
}

void setEnabled(uint64_t uuid, bool enabled) {
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

SettingItem* findByName(const std::string& path) {
    auto it = m_itemsByPath.find(path);
    return it != m_itemsByPath.end() ? &it->second : nullptr;
}

SettingItem* findByUUID(uint64_t uuid) {
    auto it = m_uuidToPath.find(uuid);
    if (it != m_uuidToPath.end()) {
        return findByName(it->second);
    }
    return nullptr;
}

private:
    LoggerSystem& m_logger; // ★ 保存參照

    std::unordered_map<std::string, SettingItem> m_itemsByPath;
    std::unordered_map<std::string, uint64_t>    m_pathToUUID;
    std::unordered_map<uint64_t, std::string>    m_uuidToPath;
};

#endif // GIRIGARA_SYSTEM_SETTINGS_REGISTRY_H
