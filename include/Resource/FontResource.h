#ifndef GIRIGARA_RESOURCE_FONT_RESOURCE_H
#define GIRIGARA_RESOURCE_FONT_RESOURCE_H

#include <string>
#include <memory>
#include <Font/FontLoader.h>

class FontResource {
public:
    FontResource(std::string resolvedPath, unsigned int fontSize);
    ~FontResource() = default;

    FontResource(const FontResource&) = delete;
    FontResource& operator=(const FontResource&) = delete;
    FontResource(FontResource&&) noexcept = default;
    FontResource& operator=(FontResource&&) noexcept = default;

    const std::string& getPath() const { return m_path; }
    unsigned int getFontSize() const { return m_fontSize; }

    FontLoader& getFontLoader() { return *m_fontLoader; }
    const FontLoader& getFontLoader() const { return *m_fontLoader; }

    bool reload(unsigned int newFontSize = 0);

private:
    std::string m_path;
    unsigned int m_fontSize;
    std::unique_ptr<FontLoader> m_fontLoader;
};

#endif // GIRIGARA_RESOURCE_FONT_RESOURCE_H