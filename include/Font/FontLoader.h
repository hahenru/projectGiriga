#ifndef GIRIGARA_FONT_FONT_LOADER_H
#define GIRIGARA_FONT_FONT_LOADER_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <vector>
#include <cstdint>

class FontLoader {
public:
    FontLoader(const std::string& fontPath, unsigned int fontSize);
    ~FontLoader();

    FontLoader(const FontLoader&) = delete;
    FontLoader& operator=(const FontLoader&) = delete;
    FontLoader(FontLoader&& other) noexcept;
    FontLoader& operator=(FontLoader&& other) noexcept;

    FT_Face getFace() const { return m_face; }
    unsigned int getFontSize() const { return m_fontSize; }

private:
    FT_Library m_ft = nullptr;
    FT_Face m_face = nullptr;
    unsigned int m_fontSize = 0;
};

class TextTexture {
public:
    TextTexture(const FontLoader& font, const std::u32string& text);
    ~TextTexture() = default;

    TextTexture(const TextTexture&) = delete;
    TextTexture& operator=(const TextTexture&) = delete;

    TextTexture(TextTexture&& other) noexcept = default;
    TextTexture& operator=(TextTexture&& other) noexcept = default;

    const std::vector<unsigned char>& getBitmap() const { return m_bitmap; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    uint64_t getRevision() const { return m_revision; }
    bool isValid() const { return !m_bitmap.empty() && m_width > 0 && m_height > 0; }

private:
    struct GlyphLayout {
        std::vector<unsigned char> buffer;
        int width;
        int rows;
        int left;
        int top;
        int penX;
    };

    void bake(FT_Face face, const std::u32string& text);

    std::vector<unsigned char> m_bitmap;
    int m_width = 0;
    int m_height = 0;
    uint64_t m_revision = 0;
};

#endif // GIRIGARA_FONT_FONT_LOADER_H