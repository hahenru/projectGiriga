#include <UI/UIText.h>
#include <Renderer/UIRenderer.h>


UIText::UIText(float x, float y, const FontLoader& font, const std::u32string& text, unsigned int fontSize)
: UIElement(x, y, 0.0f, 0.0f), m_font(font), m_text(text), m_targetFontSize(fontSize) {
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

    // 1. 取得 FontLoader 的原生烘焙基準字號
    unsigned int baseSize = m_font.getFontSize();
    if (m_targetFontSize > 0 && baseSize > 0) {
        m_scale = static_cast<float>(m_targetFontSize) / static_cast<float>(baseSize);
    } else {
        m_scale = 1.0f;
    }

    float baseW = static_cast<float>(m_texture->getWidth()) * m_scale;
    float baseH = static_cast<float>(m_texture->getHeight()) * m_scale;

    // 2. ★ 尺寸限制：如果設有父節點且字體超出父節點可用空間，強制縮小
    if (m_parent && m_parent->getWidth() > 0.0f && m_parent->getHeight() > 0.0f) {
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

    // 重新烘焙成單通道紋理
    m_texture = std::make_unique<TextTexture>(m_font, m_text);
    updateMetrics();
}

void UIText::render(UIRenderer& renderer) {
    if (!m_visible || !m_texture || m_width <= 0.0f || m_height <= 0.0f) return;

    // 透過 renderer 綁定紋理並完成繪製
    renderer.drawTexturedRect(
        getGlobalX(), getGlobalY(),
        getLocalWidth(), getLocalHeight(),
        m_texture->getID(),
        m_color
    );
}

bool UIText::handleMouseMove(double /*x*/, double /*y*/) {
    return false; // 不攔截滑鼠懸停，讓事件穿透到下方的按鈕
}

bool UIText::handleMouseButton(int /*button*/, int /*action*/, int /*mods*/, double /*x*/, double /*y*/) {
    return false; // 不攔截滑鼠點擊，讓下方的 UIButton 接收事件
}
