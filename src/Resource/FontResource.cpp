#include <Resource/FontResource.h>
#include <iostream>

FontResource::FontResource(std::string resolvedPath, unsigned int fontSize)
    : m_path(std::move(resolvedPath)),
      m_fontSize(fontSize),
      m_fontLoader(std::make_unique<FontLoader>(m_path, m_fontSize)) {}

bool FontResource::reload(unsigned int newFontSize) {
    if (newFontSize > 0) {
        m_fontSize = newFontSize;
    }

    try {
        m_fontLoader = std::make_unique<FontLoader>(m_path, m_fontSize);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[FontResource] Failed to reload font: " << m_path 
                  << " (Error: " << e.what() << ")\n";
        return false;
    }
}