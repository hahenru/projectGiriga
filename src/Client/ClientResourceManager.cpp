#include <Client/ClientResourceManager.h>
#include <System/LoggerSystem.h>
#include <fstream>
#include <sstream>

ClientResourceManager::ClientResourceManager(LoggerSystem& logger)
    : m_logger(logger) {
    registerFontPath("default", {
        "./assets/font/NaikaiFont-Regular.ttf",
        "/usr/share/fonts/google-noto-cjk-fonts/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
    }, 48);
}

// 內部多型查詢實作
IResource* ClientResourceManager::getInternal(std::type_index type, const std::string& name) {
    auto mapIt = m_registry.find(type);
    if (mapIt == m_registry.end()) return nullptr;
    auto resIt = mapIt->second.find(name);
    if (resIt == mapIt->second.end()) return nullptr;
    return resIt->second.get();
}

bool ClientResourceManager::hasInternal(std::type_index type, const std::string& name) const {
    auto mapIt = m_registry.find(type);
    if (mapIt == m_registry.end()) return false;
    return mapIt->second.find(name) != mapIt->second.end();
}

bool ClientResourceManager::reload(const std::type_index type, const std::string& name) {
    auto* res = getInternal(std::type_index(type), name);
    return res ? res->reload() : false;
}

// --- Shader 載入實作 ---
Shader& ClientResourceManager::loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath) {
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

Shader& ClientResourceManager::getShader(const std::string& name) {
    return get<Shader>(name);
}

// --- Font 載入實作 ---
void ClientResourceManager::registerFontPath(const std::string& name, const std::vector<std::string>& candidates, unsigned int defaultSize) {
    m_fontConfigs[name] = FontConfig{ candidates, defaultSize };
}

FontResource& ClientResourceManager::loadFont(const std::string& name, const std::vector<std::string>& candidates, unsigned int fontSize) {
    auto& map = m_registry[std::type_index(typeid(FontResource))];
    auto it = map.find(name);
    if (it != map.end()) {
        return *static_cast<FontResource*>(it->second.get());
    }

    std::string path = resolvePath(candidates);
    if (path.empty()) {
        throw std::runtime_error("[ClientResourceManager] Font not found: " + name);
    }

    auto font = std::make_unique<FontResource>(path, fontSize);
    auto* raw = font.get();
    map.emplace(name, std::move(font));
    return *raw;
}

FontResource& ClientResourceManager::loadFont(const std::string& name, const std::string& path, unsigned int fontSize) {
    return loadFont(name, std::vector<std::string>{path}, fontSize);
}

FontResource& ClientResourceManager::getFont(const std::string& name) {
    auto* ptr = getInternal(std::type_index(typeid(FontResource)), name);
    if (ptr) return *static_cast<FontResource*>(ptr);

    auto cfgIt = m_fontConfigs.find(name);
    if (cfgIt != m_fontConfigs.end()) {
        return loadFont(name, cfgIt->second.candidatePaths, cfgIt->second.defaultSize);
    }
    throw std::runtime_error("[ClientResourceManager] Font not registered: " + name);
}

FontLoader& ClientResourceManager::getFontLoader(const std::string& name) {
    return getFont(name).getFontLoader();
}

// --- Audio 載入實作 ---
AudioResource& ClientResourceManager::loadAudio(const std::string& name, const std::vector<std::string>& candidates) {
    auto& map = m_registry[std::type_index(typeid(AudioResource))];
    auto it = map.find(name);
    if (it != map.end()) {
        return *static_cast<AudioResource*>(it->second.get());
    }

    std::string path = resolvePath(candidates);
    if (path.empty()) {
        throw std::runtime_error("[ClientResourceManager] Audio not found: " + name);
    }

    auto audio = std::make_unique<AudioResource>(path);
    auto* raw = audio.get();
    map.emplace(name, std::move(audio));
    return *raw;
}

AudioResource& ClientResourceManager::loadAudio(const std::string& name, const std::string& path) {
    return loadAudio(name, std::vector<std::string>{path});
}

AudioResource& ClientResourceManager::getAudio(const std::string& name) {
    return get<AudioResource>(name);
}

// --- Script 載入實作 ---
ScriptResource& ClientResourceManager::loadScript(const std::string& name, const std::vector<std::string>& candidates) {
    auto& map = m_registry[std::type_index(typeid(ScriptResource))];
    auto it = map.find(name);
    if (it != map.end()) {
        return *static_cast<ScriptResource*>(it->second.get());
    }

    std::string path = resolvePath(candidates);
    if (path.empty()) {
        throw std::runtime_error("[ClientResourceManager] Script not found: " + name);
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("[ClientResourceManager] Cannot open script: " + path);
    }

    std::stringstream buf;
    buf << file.rdbuf();

    auto script = std::make_unique<ScriptResource>(path, buf.str());
    auto* raw = script.get();
    map.emplace(name, std::move(script));
    return *raw;
}

ScriptResource& ClientResourceManager::loadScript(const std::string& name, const std::string& path) {
    return loadScript(name, std::vector<std::string>{path});
}

ScriptResource& ClientResourceManager::getScript(const std::string& name) {
    return get<ScriptResource>(name);
}