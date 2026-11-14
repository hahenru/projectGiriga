#ifndef GIRIGARA_SYSTEM_SETTINGS_REGISTRY_H
#define GIRIGARA_SYSTEM_SETTINGS_REGISTRY_H

#include <Core/Depersonalization.h>
#include <Core/Identifiable.h>
#include <Event/EventBus.h>
#include <System/LoggerSystem.h>
#include <UI/UIElement.h>
#include <string>
#include <unordered_map>
#include <functional>
#include <variant>
#include <cstdint>

using SettingValue = std::variant<bool, int, float, std::string>;

struct SettingItem {
    Depersonalization id;
    std::string className;
    std::string groupName;
    std::string keyName;
    std::string fullPath;

    bool isEnabled = true;

    std::function<void(const SettingValue&)> setter;
    std::function<SettingValue()> getter;

    UIElement* boundUIElement = nullptr;
};

class SettingsRegistry : public Identifiable {
public:
    SettingsRegistry(EventBus& eventBus, LoggerSystem& logger);
    ~SettingsRegistry() override;

    SettingsRegistry(const SettingsRegistry&) = delete;
    SettingsRegistry& operator=(const SettingsRegistry&) = delete;

    uint64_t registerSetting(const std::string& className,
                             const std::string& groupName,
                             const std::string& keyName,
                             std::function<void(const SettingValue&)> setter,
                             std::function<SettingValue()> getter,
                             UIElement* uiElement = nullptr);

    void bindUIElement(uint64_t uuid, UIElement* elem);

    bool setByName(const std::string& path, const SettingValue& val, bool force = true);
    bool setByUUID(uint64_t uuid, const SettingValue& val, bool force = true);
    void setEnabled(uint64_t uuid, bool enabled);

    SettingItem* findByName(const std::string& path);
    SettingItem* findByUUID(uint64_t uuid);

private:
    void registerScriptBindings();
    void unregisterScriptBindings();
    void bindEvents();

    EventBus& m_eventBus;
    LoggerSystem& m_logger;

    std::unordered_map<std::string, SettingItem> m_itemsByPath;
    std::unordered_map<std::string, uint64_t>    m_pathToUUID;
    std::unordered_map<uint64_t, std::string>    m_uuidToPath;
};

#endif // GIRIGARA_SYSTEM_SETTINGS_REGISTRY_H
