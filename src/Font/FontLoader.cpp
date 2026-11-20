#include <Font/FontLoader.h>

#include <stdexcept>
#include <algorithm>
#include <utility>

// ==========================================
// FontLoader 實作
// ==========================================

FontLoader::FontLoader(const std::string& fontPath, unsigned int fontSize)
    : m_fontSize(fontSize) {
    if (FT_Init_FreeType(&m_ft)) {
        throw std::runtime_error("Failed to initialize FreeType library");
    }

    if (FT_New_Face(m_ft, fontPath.c_str(), 0, &m_face)) {
        FT_Done_FreeType(m_ft);
        m_ft = nullptr;
        throw std::runtime_error("Failed to load font from: " + fontPath);
    }

    FT_Set_Pixel_Sizes(m_face, 0, fontSize);
}

FontLoader::~FontLoader() {
    if (m_face) FT_Done_Face(m_face);
    if (m_ft) FT_Done_FreeType(m_ft);
}

FontLoader::FontLoader(FontLoader&& other) noexcept
    : m_ft(other.m_ft), m_face(other.m_face), m_fontSize(other.m_fontSize) {
    other.m_ft = nullptr;
    other.m_face = nullptr;
    other.m_fontSize = 0;
}

FontLoader& FontLoader::operator=(FontLoader&& other) noexcept {
    if (this != &other) {
        if (m_face) {
            FT_Done_Face(m_face);
            m_face = nullptr;
        }
        if (m_ft) {
            FT_Done_FreeType(m_ft);
            m_ft = nullptr;
        }

        m_ft = other.m_ft;
        m_face = other.m_face;
        m_fontSize = other.m_fontSize;

        other.m_ft = nullptr;
        other.m_face = nullptr;
        other.m_fontSize = 0;
    }
    return *this;
}

// ==========================================
// TextTexture 實作
// ==========================================

TextTexture::TextTexture(const FontLoader& font, const std::u32string& text) {
    bake(font.getFace(), text);
}

void TextTexture::bake(FT_Face face, const std::u32string& text) {
    m_bitmap.clear();
    m_width = 0;
    m_height = 0;
    if (text.empty()) return;

    std::vector<GlyphLayout> glyphs;
    glyphs.reserve(text.size());

    int currentPenX = 0;
    int maxAscent = 0;
    int maxDescent = 0;

    // 第一階段：度量與擷取各字元點陣資料
    for (char32_t codepoint : text) {
        if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) {
            continue;
        }

        FT_GlyphSlot slot = face->glyph;
        GlyphLayout layout;
        layout.width = slot->bitmap.width;
        layout.rows = slot->bitmap.rows;
        layout.left = slot->bitmap_left;
        layout.top = slot->bitmap_top;
        layout.penX = currentPenX;

        if (layout.width > 0 && layout.rows > 0) {
            layout.buffer.assign(slot->bitmap.buffer,
                                 slot->bitmap.buffer + (layout.width * layout.rows));
        }

        maxAscent = std::max(maxAscent, layout.top);
        maxDescent = std::max(maxDescent, layout.rows - layout.top);

        currentPenX += (slot->advance.x >> 6);
        glyphs.push_back(std::move(layout));
    }

    m_width = currentPenX;
    m_height = maxAscent + maxDescent;

    if (m_width <= 0 || m_height <= 0) return;

    // 第二階段：將所有字元合成進同一張單通道緩衝區
    std::vector<unsigned char> mergedBuffer(static_cast<size_t>(m_width) * m_height, 0);

    for (const auto& g : glyphs) {
        if (g.buffer.empty()) continue;

        int dstStartX = g.penX + g.left;
        int dstStartY = maxAscent - g.top;

        for (int r = 0; r < g.rows; ++r) {
            for (int c = 0; c < g.width; ++c) {
                int dstX = dstStartX + c;
                int dstY = dstStartY + r;

                if (dstX >= 0 && dstX < m_width && dstY >= 0 && dstY < m_height) {
                    unsigned char srcVal = g.buffer[r * g.width + c];
                    int dstIdx = dstY * m_width + dstX;
                    mergedBuffer[dstIdx] = std::max(mergedBuffer[dstIdx], srcVal);
                }
            }
        }
    }

    // 第三階段：保存 CPU 端點陣，交由 UIRenderer 懶上傳至 VulkanTexture
    m_bitmap = std::move(mergedBuffer);
    ++m_revision;
}