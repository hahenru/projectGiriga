#include <UI/UIFactory.h>
#include <UI/UIButton.h>
#include <UI/UIText.h>
#include <UI/UIInputField.h>
#include <UI/UIPanel.h>
#include <UI/UIWindow.h>

void UIFactory::registerType(const std::string& typeName, CreatorFunc creator) {
    m_creators[typeName] = std::move(creator);
}

void UIFactory::unregisterType(const std::string& typeName) {
    m_creators.erase(typeName);
}

std::unique_ptr<UIElement> UIFactory::create(const std::string& typeName,
const UIBuildContext& ctx,
const UIPropMap& props) const {
    auto it = m_creators.find(typeName);
    if (it == m_creators.end()) return nullptr;
    return it->second(ctx, props);
}

float UIFactory::getFloat(const UIPropMap& props, const std::string& key, float fallback) {
    auto it = props.find(key);
    if (it == props.end()) return fallback;
    if (auto* f = std::get_if<float>(&it->second)) return *f;
    if (auto* i = std::get_if<int>(&it->second))   return static_cast<float>(*i);
    return fallback;
}

int UIFactory::getInt(const UIPropMap& props, const std::string& key, int fallback) {
    auto it = props.find(key);
    if (it == props.end()) return fallback;
    if (auto* i = std::get_if<int>(&it->second))   return *i;
    if (auto* f = std::get_if<float>(&it->second)) return static_cast<int>(*f);
    return fallback;
}

std::string UIFactory::getString(const UIPropMap& props, const std::string& key, const std::string& fallback) {
    auto it = props.find(key);
    if (it == props.end()) return fallback;
    if (auto* s = std::get_if<std::string>(&it->second)) return *s;
    return fallback;
}

bool UIFactory::getBool(const UIPropMap& props, const std::string& key, bool fallback) {
    auto it = props.find(key);
    if (it == props.end()) return fallback;
    if (auto* b = std::get_if<bool>(&it->second)) return *b;
    return fallback;
}

void UIFactory::registerBuiltinTypes() {
    // 1. 按鈕 (UIButton)
    registerType("Button", [](const UIBuildContext& ctx, const UIPropMap& p) {
        auto btn = std::make_unique<UIButton>(
            getFloat(p, "x"),
            getFloat(p, "y"),
            getFloat(p, "w", 120.0f),
            getFloat(p, "h", 36.0f)
        );
        std::string text = getString(p, "text");
        if (!text.empty()) {
            btn->setLabel(ctx.resources, EncodingUtils::utf8ToUtf32(text));
        }
        return btn;
    });

    // 2. 文字標籤 (UIText)
    registerType("Text", [](const UIBuildContext& ctx, const UIPropMap& p) {
        std::string text = getString(p, "text");
        unsigned int fontSize = static_cast<unsigned int>(getInt(p, "fontSize", 0));
        std::string fontName = getString(p, "font", "default");
        return std::make_unique<UIText>(
            getFloat(p, "x"),
            getFloat(p, "y"),
            ctx.resources,
            EncodingUtils::utf8ToUtf32(text),
            fontSize,
            std::move(fontName)
        );
    });

    // 3. 文字輸入框 (UIInputField)
    registerType("InputField", [](const UIBuildContext& ctx, const UIPropMap& p) {
        unsigned int fontSize = static_cast<unsigned int>(getInt(p, "fontSize", 18));
        return std::make_unique<UIInputField>(
            getFloat(p, "x"),
            getFloat(p, "y"),
            getFloat(p, "w", 200.0f),
            getFloat(p, "h", 32.0f),
            ctx.resources,
            ctx.eventBus,
            fontSize
        );
    });

    // 4. 面板與視窗 (UIPanel / UIWindow)
    registerType("Panel", [](const UIBuildContext&, const UIPropMap& p) {
        return std::make_unique<UIPanel>(
            getFloat(p, "x"),
            getFloat(p, "y"),
            getFloat(p, "w", 200.0f),
            getFloat(p, "h", 200.0f)
        );
    });

    registerType("Window", [](const UIBuildContext& ctx, const UIPropMap& p) {
        std::string title    = getString(p, "title", "Window");
        unsigned int fontSz  = static_cast<unsigned int>(getInt(p, "titleFontSize", 0));
        std::string fontName = getString(p, "font", "default");

        return std::make_unique<UIWindow>(
            getFloat(p, "x"),
                                          getFloat(p, "y"),
                                          getFloat(p, "w", 320.0f),
                                          getFloat(p, "h", 240.0f),
                                          ctx.resources,
                                          EncodingUtils::utf8ToUtf32(title),
                                          fontSz,
                                          std::move(fontName)
        );
    });
}
