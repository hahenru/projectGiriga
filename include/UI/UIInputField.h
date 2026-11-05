#ifndef GIRIGARA_UI_UI_INPUT_FIELD_H
#define GIRIGARA_UI_UI_INPUT_FIELD_H

#include <UI/UIContainer.h>
#include <UI/UIText.h>
#include <Event/EventBus.h>
#include <Renderer/UIRenderer.h>
#include <Resource/ResourceManager.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

namespace EncodingUtils {
    inline std::string utf32ToUtf8(const std::u32string& str) {
        std::string out;
        for (char32_t cp : str) {
            if (cp <= 0x7F) {
                out.push_back(static_cast<char>(cp));
            } else if (cp <= 0x7FF) {
                out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp <= 0xFFFF) {
                out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp <= 0x10FFFF) {
                out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }
        return out;
    }

    inline std::u32string utf8ToUtf32(const std::string& str) {
        std::u32string out;
        size_t i = 0;
        while (i < str.size()) {
            unsigned char c = str[i];
            char32_t cp = 0;
            size_t bytes = 0;

            if (c < 0x80) {
                cp = c;
                bytes = 1;
            } else if ((c & 0xE0) == 0xC0) {
                cp = c & 0x1F;
                bytes = 2;
            } else if ((c & 0xF0) == 0xE0) {
                cp = c & 0x0F;
                bytes = 3;
            } else if ((c & 0xF8) == 0xF0) {
                cp = c & 0x07;
                bytes = 4;
            } else {
                i++;
                continue;
            }

            if (i + bytes > str.size()) break;

            for (size_t j = 1; j < bytes; ++j) {
                cp = (cp << 6) | (str[i + j] & 0x3F);
            }
            out.push_back(cp);
            i += bytes;
        }
        return out;
    }
}

class UIInputField : public UIContainer {
public:
    using SubmitCallback = std::function<void(const std::string&)>;

	UIInputField(float x, float y, float width, float height,
				 ResourceManager& resources, EventBus& eventBus, unsigned int fontSize = 18,
				 std::string fontName = "default")
		: UIContainer(x, y, width, height),
		  m_resources(resources),
		  m_eventBus(eventBus),
		  m_fontSize(fontSize),
		  m_fontName(std::move(fontName)) {
		
		m_textLabel = emplaceChild<UIText>(
			6.0f, (height - static_cast<float>(fontSize)) * 0.5f,
			m_resources, U"", fontSize, m_fontName
		);
		
        m_textLabel->setColor(1.0f, 1.0f, 1.0f, 1.0f);

        bindEvents();
    }

    void setOnSubmit(SubmitCallback callback) { m_onSubmit = std::move(callback); }

    void setText(const std::string& text) {
        m_text = text;
        syncTextLabel();
    }

    const std::string& getText() const { return m_text; }

    void update(float dt) override {
        UIContainer::update(dt);
        if (isFocused()) {
            m_cursorBlinkTime += dt;
            if (m_cursorBlinkTime >= 0.5f) {
                m_cursorVisible = !m_cursorVisible;
                m_cursorBlinkTime = 0.0f;
            }
        } else {
            m_cursorVisible = false;
        }
    }

    void render(UIRenderer& renderer) override {
        if (!isVisible()) return;

        // 1. 繪製輸入框底色
        renderer.drawQuad(getGlobalX(), getGlobalY(), getWidth(), getHeight(), 0.1f, 0.1f, 0.1f, 0.7f);

        // 2. 透過 UIContainer 繪製 UIText
        UIContainer::render(renderer);

        // 3. 游標渲染
        if (m_cursorVisible) {
            float textWidth = (m_textLabel && m_textLabel->getWidth() > 0.0f) ? m_textLabel->getWidth() : 0.0f;
            float cursorX = getGlobalX() + 6.0f + textWidth + 2.0f;
            float cursorY = getGlobalY() + (getHeight() - static_cast<float>(m_fontSize)) * 0.5f;
            renderer.drawQuad(cursorX, cursorY, 2.0f, static_cast<float>(m_fontSize), 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    bool handleMouseButton(int button, int action, int mods, double x, double y) override {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            setFocused(contains(x, y));
            return isFocused();
        }
        return false;
    }

    bool handleMouseMove(double, double) override { return false; }

private:
    void syncTextLabel() {
        if (m_textLabel) {
            m_textLabel->setText(EncodingUtils::utf8ToUtf32(m_text));
        }
    }

	void bindEvents() {
        m_eventBus.subscribePriority<CharInputEvent>([this](CharInputEvent& e) {
            if (!this->isFocused() || !this->isVisible()) return;

            // 阻擋觸發 Console 的快捷鍵字元（` 與 ~）
            if (e.codepoint == '`' || e.codepoint == '~') {
                e.isHandled = true;
                return;
            }

            if (e.codepoint >= 32) {
                std::u32string u32str(1, static_cast<char32_t>(e.codepoint));
                // ★ 修正：u32string 轉 string 要用 utf32ToUtf8
                m_text += EncodingUtils::utf32ToUtf8(u32str);
                syncTextLabel();
                e.isHandled = true;
            }
        });

        m_eventBus.subscribePriority<KeyEvent>([this](KeyEvent& e) {
            if (!this->isFocused() || !this->isVisible()) return;
            if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
                if (e.key == GLFW_KEY_BACKSPACE && !m_text.empty()) {
                    auto u32 = EncodingUtils::utf8ToUtf32(m_text);
                    if (!u32.empty()) {
                        u32.pop_back();
                        // ★ 修正：u32string 轉回 string 要用 utf32ToUtf8
                        m_text = EncodingUtils::utf32ToUtf8(u32);
                        syncTextLabel();
                    }
                    e.isHandled = true;
                } else if (e.key == GLFW_KEY_ENTER) {
                    if (m_onSubmit) {
                        m_onSubmit(m_text);
                    }
                    m_text.clear();
                    syncTextLabel();
                    e.isHandled = true;
                }
            }
        });
    }

    ResourceManager& m_resources;
	std::string m_fontName;
    EventBus& m_eventBus;
    UIText* m_textLabel{nullptr};
    SubmitCallback m_onSubmit;
    std::string m_text;
    unsigned int m_fontSize;
    float m_cursorBlinkTime = 0.0f;
    bool m_cursorVisible = false;
};

#endif // GIRIGARA_UI_UI_INPUT_FIELD_H