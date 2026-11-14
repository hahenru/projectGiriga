#include <UI/UIText.h>
#include <Renderer/UIRenderer.h>
#include <Resource/FontResource.h>
#include <algorithm>

UIText::UIText(float x, float y, IResourceManager& resources,
               const std::u32string& text, unsigned int fontSize,
               std::string fontName)
    : UIElement(x, y, 0.0f, 0.0f),
      m_resources(resources),
      m_fontName(std::move(fontName)),
      m_text(text),
      m_targetFontSize(fontSize) {
    rebake();
}

void UIText::setText(const std::u32string& text) {
    if (m_text != text) {
        m_text = text;
        rebake();
    }
}

void UIText::setFontSize(unsigned int fontSize) {
    if (m_targetFontSize != fontSize) {
        m_targetFontSize = fontSize;
        updateMetrics();
    }
}

void UIText::setFontName(const std::string& fontName) {
    if (m_fontName != fontName) {
        m_fontName = fontName;
        rebake();
    }
}

void UIText::setColor(float r, float g, float b, float a) {
    m_color[0] = r;
    m_color[1] = g;
    m_color[2] = b;
    m_color[3] = a;
}


void UIText::updateMetrics() {
	
    if (!m_texture) {
        m_width = 0.0f;
        m_height = 0.0f;
        m_scale = 1.0f;
        return;
    }

    FontLoader& font = m_resources.get<FontResource>(m_fontName).getFontLoader();
    unsigned int baseSize = font.getFontSize();
    if (m_targetFontSize > 0 && baseSize > 0) {
        m_scale = static_cast<float>(m_targetFontSize) / static_cast<float>(baseSize);
    } else {
        m_scale = 1.0f;
    }

    float baseW = static_cast<float>(m_texture->getWidth()) * m_scale;
    float baseH = static_cast<float>(m_texture->getHeight()) * m_scale;

    if (m_autoShrink && m_parent && m_parent->getWidth() > 0.0f && m_parent->getHeight() > 0.0f) {
        float maxAvailableW = m_parent->getWidth();
        float maxAvailableH = m_parent->getHeight();

        float shrinkFactor = 1.0f;
        if (baseW > maxAvailableW) {
            shrinkFactor = std::min(shrinkFactor, maxAvailableW / baseW);
        }
        if (baseH > maxAvailableH) {
            shrinkFactor = std::min(shrinkFactor, maxAvailableH / baseH);
        }

        m_scale *= shrinkFactor;
        m_width = baseW * shrinkFactor;
        m_height = baseH * shrinkFactor;
    } else {
        m_width = baseW;
        m_height = baseH;
    }
}

void UIText::rebake() {
    if (m_text.empty()) {
        m_texture.reset();
        m_width = 0.0f;
        m_height = 0.0f;
        return;
    }

    FontLoader& font = m_resources.get<FontResource>(m_fontName).getFontLoader();
    m_texture = std::make_unique<TextTexture>(font, m_text);
    updateMetrics();
}

void UIText::render(UIRenderer& renderer) {
    if (!m_visible || !m_texture || m_width <= 0.0f || m_height <= 0.0f) return;

    renderer.drawTexturedRect(
        getGlobalX(), getGlobalY(),
        getLocalWidth(), getLocalHeight(),
        m_texture->getID(),
        m_color
    );
}
void UIText::setAutoShrink(bool enabled) {
    if (m_autoShrink != enabled) {
        m_autoShrink = enabled;
        updateMetrics();
    }
}

bool UIText::handleMouseMove(double, double) { return false; }
bool UIText::handleMouseButton(int, int, int, double, double) { return false; }