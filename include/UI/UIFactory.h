#ifndef GIRIGARA_UI_UI_FACTORY_H
#define GIRIGARA_UI_UI_FACTORY_H

#include <UI/UIElement.h>
#include <Resource/IResourceManager.h>
#include <Event/EventBus.h>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <variant>

using UIPropValue = std::variant<bool, int, float, std::string>;
using UIPropMap   = std::unordered_map<std::string, UIPropValue>;

struct UIBuildContext {
    IResourceManager& resources;
    EventBus& eventBus;
};

class UIFactory {
public:
    using CreatorFunc = std::function<std::unique_ptr<UIElement>(const UIBuildContext&, const UIPropMap&)>;

    UIFactory() = default;
    ~UIFactory() = default;

    // 註冊引擎內建控制項（具體控制項依賴全數封裝在 UIFactory.cpp）
    void registerBuiltinTypes();

    void registerType(const std::string& typeName, CreatorFunc creator);
    void unregisterType(const std::string& typeName);

    std::unique_ptr<UIElement> create(const std::string& typeName,
                                      const UIBuildContext& ctx,
                                      const UIPropMap& props = {}) const;

                                      // 輔助取值工具
                                      static float getFloat(const UIPropMap& props, const std::string& key, float fallback = 0.0f);
                                      static int getInt(const UIPropMap& props, const std::string& key, int fallback = 0);
                                      static std::string getString(const UIPropMap& props, const std::string& key, const std::string& fallback = "");
                                      static bool getBool(const UIPropMap& props, const std::string& key, bool fallback = false);

private:
    std::unordered_map<std::string, CreatorFunc> m_creators;
};

#endif // GIRIGARA_UI_UI_FACTORY_H
