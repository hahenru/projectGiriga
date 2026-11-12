#ifndef GIRIGARA_UI_UI_TEXT_H
#define GIRIGARA_UI_UI_TEXT_H

#include <UI/UIElement.h>
#include <Resource/IResourceManager.h>
#include <Font/FontLoader.h>
#include <memory>
#include <string>

class UIRenderer;

class UIText : public UIElement {
public:
    // ★ 現代化建構：由 ResourceManager 依 fontName 取得字型
    UIText(float x, float y, IResourceManager& resources,
           const std::u32string& text, unsigned int fontSize = 0,
           std::string fontName = "default");

    ~UIText() override = default;

    void setText(const std::u32string& text);
    const std::u32string& getText() const { return m_text; }

    void setFontSize(unsigned int fontSize);
    unsigned int getFontSize() const { return m_targetFontSize; }

    // 支援動態切換字型族
    void setFontName(const std::string& fontName);

    void setColor(float r, float g, float b, float a = 1.0f);

    void render(UIRenderer& renderer) override;
    bool handleMouseMove(double x, double y) override;
    bool handleMouseButton(int button, int action, int mods, double x, double y) override;
	void setAutoShrink(bool enabled);


private:
    IResourceManager& m_resources;
    std::string m_fontName;
    std::u32string m_text;
    std::unique_ptr<TextTexture> m_texture = nullptr;
    float m_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    unsigned int m_targetFontSize = 0;
    float m_scale = 1.0f;
	bool m_autoShrink = true;

    void rebake();
    void updateMetrics();
};

#endif // GIRIGARA_UI_UI_TEXT_H