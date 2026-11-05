#include <Resource/ResourceManager.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

ResourceManager::ResourceManager(LoggerSystem& logger) : m_logger(logger) {
    registerFontPath("default", {
        "./assets/font/NaikaiFont-Regular.ttf",
        "/usr/share/fonts/google-noto-cjk-fonts/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
    }, 48);
}

std::string ResourceManager::resolvePath(const std::vector<std::string>& candidates) const {
    for (const auto& path : candidates) {
        if (fs::exists(path)) {
            return path;
        }
    }
    return "";
}

// -------------------------------------------------------------
// Shader
// -------------------------------------------------------------
Shader& ResourceManager::loadShader(const std::string& name,
                                    const std::string& vertPath,
                                    const std::string& fragPath) {
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return *(it->second);
    }

    auto shader = std::make_unique<Shader>(vertPath, fragPath);
    auto& ref = *shader;
    m_shaders.emplace(name, std::move(shader));
    return ref;
}

Shader& ResourceManager::getShader(const std::string& name) {
    auto it = m_shaders.find(name);
    if (it == m_shaders.end()) {
        throw std::runtime_error("[ResourceManager] Shader not found: " + name);
    }
    return *(it->second);
}

// -------------------------------------------------------------
// Font
// -------------------------------------------------------------
void ResourceManager::registerFontPath(const std::string& name,
                                      const std::vector<std::string>& candidatePaths,
                                      unsigned int defaultSize) {
    m_fontConfigs[name] = FontConfig{ candidatePaths, defaultSize };
}

FontResource& ResourceManager::loadFont(const std::string& name,
                                        const std::vector<std::string>& candidatePaths,
                                        unsigned int fontSize) {
    auto it = m_fonts.find(name);
    if (it != m_fonts.end()) {
        return *(it->second);
    }

    std::string validPath = resolvePath(candidatePaths);
    if (validPath.empty()) {
        throw std::runtime_error("[ResourceManager] No valid font file found for: " + name);
    }

    auto font = std::make_unique<FontResource>(validPath, fontSize);
    auto& ref = *font;
    m_fonts.emplace(name, std::move(font));
    return ref;
}

FontResource& ResourceManager::loadFont(const std::string& name,
                                        const std::string& path,
                                        unsigned int fontSize) {
    return loadFont(name, std::vector<std::string>{path}, fontSize);
}

FontResource& ResourceManager::getFont(const std::string& name) {
    auto it = m_fonts.find(name);
    if (it != m_fonts.end()) {
        return *(it->second);
    }

    auto cfgIt = m_fontConfigs.find(name);
    if (cfgIt != m_fontConfigs.end()) {
        return loadFont(name, cfgIt->second.candidatePaths, cfgIt->second.defaultSize);
    }

    throw std::runtime_error("[ResourceManager] Font not found and no registered paths for: " + name);
}

FontLoader& ResourceManager::getFontLoader(const std::string& name) {
    return getFont(name).getFontLoader();
}

bool ResourceManager::reloadFont(const std::string& name, unsigned int newFontSize) {
    auto it = m_fonts.find(name);
    if (it == m_fonts.end()) {
        std::cerr << "[ResourceManager] Cannot reload non-existent font: " << name << "\n";
        return false;
    }
    return it->second->reload(newFontSize);
}

// -------------------------------------------------------------
// Audio
// -------------------------------------------------------------
AudioResource& ResourceManager::loadAudio(const std::string& name,
                                          const std::vector<std::string>& candidatePaths) {
    auto it = m_audios.find(name);
    if (it != m_audios.end()) {
        return *(it->second);
    }

    std::string validPath = resolvePath(candidatePaths);
    if (validPath.empty()) {
        throw std::runtime_error("[ResourceManager] No valid audio file found for: " + name);
    }

    auto audio = std::make_unique<AudioResource>(validPath);
    if (!audio->isLoaded()) {
        throw std::runtime_error("[ResourceManager] Failed to decode audio: " + validPath);
    }

    auto& ref = *audio;
    m_audios.emplace(name, std::move(audio));
    return ref;
}

AudioResource& ResourceManager::loadAudio(const std::string& name,
                                          const std::string& path) {
    return loadAudio(name, std::vector<std::string>{path});
}

AudioResource& ResourceManager::getAudio(const std::string& name) {
    auto it = m_audios.find(name);
    if (it == m_audios.end()) {
        throw std::runtime_error("[ResourceManager] Audio not found: " + name);
    }
    return *(it->second);
}

bool ResourceManager::reloadAudio(const std::string& name) {
    auto it = m_audios.find(name);
    if (it == m_audios.end()) {
        std::cerr << "[ResourceManager] Cannot reload non-existent audio: " << name << "\n";
        return false;
    }
    return it->second->reload();
}

// -------------------------------------------------------------
// Script
// -------------------------------------------------------------
ScriptResource& ResourceManager::loadScript(const std::string& name,
                                            const std::vector<std::string>& candidatePaths) {
    auto it = m_scripts.find(name);
    if (it != m_scripts.end()) {
        return *(it->second);
    }

    std::string validPath = resolvePath(candidatePaths);
    if (validPath.empty()) {
        throw std::runtime_error("[ResourceManager] Script candidate paths not found for: " + name);
    }

    std::ifstream file(validPath);
    if (!file.is_open()) {
        throw std::runtime_error("[ResourceManager] Cannot open script file: " + validPath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    auto script = std::make_unique<ScriptResource>(validPath, buffer.str());
    ScriptResource& ref = *script;
    m_scripts.emplace(name, std::move(script));
    return ref;
}

ScriptResource& ResourceManager::loadScript(const std::string& name,
                                            const std::string& path) {
    return loadScript(name, std::vector<std::string>{path});
}

ScriptResource& ResourceManager::getScript(const std::string& name) {
    auto it = m_scripts.find(name);
    if (it == m_scripts.end()) {
        throw std::runtime_error("[ResourceManager] Script not found: " + name);
    }
    return *(it->second);
}

bool ResourceManager::reloadScript(const std::string& name) {
    auto it = m_scripts.find(name);
    if (it == m_scripts.end()) {
        std::cerr << "[ResourceManager] Cannot reload non-existent script: " << name << "\n";
        return false;
    }
    return it->second->reload();
}