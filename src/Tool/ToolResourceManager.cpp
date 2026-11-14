#include <Tool/ToolResourceManager.h>
#include <System/LoggerSystem.h>
#include <fstream>
#include <sstream>
#include <algorithm>

ToolResourceManager::ToolResourceManager(LoggerSystem& logger)
: m_logger(logger) {
    registerFontPath("default", {
        "./assets/font/NaikaiFont-Regular.ttf",
        "/usr/share/fonts/google-noto-cjk-fonts/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
    }, 48);
}

IResource* ToolResourceManager::getInternal(std::type_index type, const std::string& name) {
    auto mapIt = m_registry.find(type);
    if (mapIt == m_registry.end()) return nullptr;
    auto resIt = mapIt->second.find(name);
    if (resIt == mapIt->second.end()) return nullptr;
    return resIt->second.get();
}

bool ToolResourceManager::hasInternal(std::type_index type, const std::string& name) const {
    auto mapIt = m_registry.find(type);
    if (mapIt == m_registry.end()) return false;
    return mapIt->second.find(name) != mapIt->second.end();
}

bool ToolResourceManager::reload(const std::type_index type, const std::string& name) {
    auto* res = getInternal(type, name);
    return res ? res->reload() : false;
}

// --- Shader ---
Shader& ToolResourceManager::loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath) {
    auto& map = m_registry[std::type_index(typeid(Shader))];
    auto it = map.find(name);
    if (it != map.end()) {
        return *static_cast<Shader*>(it->second.get());
    }
    auto shader = std::make_unique<Shader>(vertPath, fragPath);
    auto* raw = shader.get();
    map.emplace(name, std::move(shader));
    return *raw;
}

Shader& ToolResourceManager::getShader(const std::string& name) {
    return get<Shader>(name);
}

// --- Font ---
void ToolResourceManager::registerFontPath(const std::string& name, const std::vector<std::string>& candidates, unsigned int defaultSize) {
    m_fontConfigs[name] = FontConfig{ candidates, defaultSize };
}

FontResource& ToolResourceManager::loadFont(const std::string& name, const std::vector<std::string>& candidates, unsigned int fontSize) {
    auto& map = m_registry[std::type_index(typeid(FontResource))];
    auto it = map.find(name);
    if (it != map.end()) {
        return *static_cast<FontResource*>(it->second.get());
    }

    std::string path = resolvePath(candidates);
    if (path.empty()) {
        throw std::runtime_error("[ToolResourceManager] Font not found: " + name);
    }

    auto font = std::make_unique<FontResource>(path, fontSize);
    auto* raw = font.get();
    map.emplace(name, std::move(font));
    return *raw;
}

FontResource& ToolResourceManager::loadFont(const std::string& name, const std::string& path, unsigned int fontSize) {
    return loadFont(name, std::vector<std::string>{path}, fontSize);
}

FontResource& ToolResourceManager::getFont(const std::string& name) {
    auto* ptr = getInternal(std::type_index(typeid(FontResource)), name);
    if (ptr) return *static_cast<FontResource*>(ptr);

    auto cfgIt = m_fontConfigs.find(name);
    if (cfgIt != m_fontConfigs.end()) {
        return loadFont(name, cfgIt->second.candidatePaths, cfgIt->second.defaultSize);
    }
    throw std::runtime_error("[ToolResourceManager] Font not registered: " + name);
}

FontLoader& ToolResourceManager::getFontLoader(const std::string& name) {
    return getFont(name).getFontLoader();
}

// --- Script ---
ScriptResource& ToolResourceManager::loadScript(const std::string& name, const std::vector<std::string>& candidates) {
    auto& map = m_registry[std::type_index(typeid(ScriptResource))];
    auto it = map.find(name);
    if (it != map.end()) {
        return *static_cast<ScriptResource*>(it->second.get());
    }

    auto tryLoadFromArchive = [&]() -> std::unique_ptr<ScriptResource> {
        std::vector<uint8_t> bytes;
        if (extractFromArchives(name, bytes)) {
            return std::make_unique<ScriptResource>(name, std::string(bytes.begin(), bytes.end()));
        }
        for (const auto& candidate : candidates) {
            if (extractFromArchives(candidate, bytes)) {
                return std::make_unique<ScriptResource>(candidate, std::string(bytes.begin(), bytes.end()));
            }
        }
        return nullptr;
    };

    std::unique_ptr<ScriptResource> script;

    if (m_preferArchive) {
        script = tryLoadFromArchive();
    }

    if (!script) {
        std::string path = resolvePath(candidates);
        if (!path.empty()) {
            std::ifstream file(path);
            if (file.is_open()) {
                std::stringstream buf;
                buf << file.rdbuf();
                script = std::make_unique<ScriptResource>(path, buf.str());
            }
        }
    }

    if (!script && !m_preferArchive) {
        script = tryLoadFromArchive();
    }

    if (!script) {
        throw std::runtime_error("[ToolResourceManager] Script not found on disk or archive: " + name);
    }

    auto* raw = script.get();
    map.emplace(name, std::move(script));
    return *raw;
}

ScriptResource& ToolResourceManager::loadScript(const std::string& name, const std::string& path) {
    return loadScript(name, std::vector<std::string>{path});
}

ScriptResource& ToolResourceManager::getScript(const std::string& name) {
    return get<ScriptResource>(name);
}

// --- Image ---
ImageResource& ToolResourceManager::loadImage(const std::string& name, const std::vector<std::string>& candidates, bool flipVertically) {
    auto& map = m_registry[std::type_index(typeid(ImageResource))];
    auto it = map.find(name);
    if (it != map.end()) {
        return *static_cast<ImageResource*>(it->second.get());
    }

    auto tryLoadFromArchive = [&]() -> std::unique_ptr<ImageResource> {
        std::vector<uint8_t> bytes;
        if (extractFromArchives(name, bytes)) {
            return std::make_unique<ImageResource>(name, bytes, flipVertically);
        }
        for (const auto& candidate : candidates) {
            if (extractFromArchives(candidate, bytes)) {
                return std::make_unique<ImageResource>(candidate, bytes, flipVertically);
            }
        }
        return nullptr;
    };

    std::unique_ptr<ImageResource> image;

    if (m_preferArchive) {
        image = tryLoadFromArchive();
    }

    if (!image) {
        std::string path = resolvePath(candidates);
        if (!path.empty()) {
            image = std::make_unique<ImageResource>(path, flipVertically);
        }
    }

    if (!image && !m_preferArchive) {
        image = tryLoadFromArchive();
    }

    if (!image || !image->isLoaded()) {
        throw std::runtime_error("[ToolResourceManager] Failed to load image from disk or archive: " + name);
    }

    auto* raw = image.get();
    map.emplace(name, std::move(image));
    return *raw;
}

ImageResource& ToolResourceManager::loadImage(const std::string& name, const std::string& path, bool flipVertically) {
    return loadImage(name, std::vector<std::string>{path}, flipVertically);
}

ImageResource& ToolResourceManager::getImage(const std::string& name) {
    return get<ImageResource>(name);
}

// --- GpkArchive 掛載實作 ---
bool ToolResourceManager::mountArchive(const std::string& archivePath, const GpkArchive::Key256& key) {
    if (std::find(m_mountedPaths.begin(), m_mountedPaths.end(), archivePath) != m_mountedPaths.end()) {
        return true;
    }

    auto archive = std::make_unique<GpkArchive>();
    if (!archive->open(archivePath, key)) {
        return false;
    }

    m_archives.push_back(std::move(archive));
    m_mountedPaths.push_back(archivePath);
    return true;
}

bool ToolResourceManager::extractFromArchives(const std::string& virtualPath, std::vector<uint8_t>& outBytes) const {
    for (auto it = m_archives.rbegin(); it != m_archives.rend(); ++it) {
        if ((*it)->extractFile(virtualPath, outBytes)) {
            return true;
        }
    }
    return false;
}
