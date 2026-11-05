#ifndef GIRIGARA_RESOURCE_RESOURCE_MANAGER_H
#define GIRIGARA_RESOURCE_RESOURCE_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <filesystem>

#include <Shader/Shader.h>
#include <Resource/FontResource.h>
#include <Resource/AudioResource.h>
#include <Resource/ScriptResource.h>

class LoggerSystem;

class ResourceManager {
public:
    explicit ResourceManager(LoggerSystem& logger);
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) noexcept = default;
    ResourceManager& operator=(ResourceManager&&) noexcept = default;

    // --- Shader ---
    Shader& loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    Shader& getShader(const std::string& name);

    // --- Font ---
    void registerFontPath(const std::string& name, const std::vector<std::string>& candidatePaths, unsigned int defaultSize = 48);
    FontResource& loadFont(const std::string& name, const std::vector<std::string>& candidatePaths, unsigned int fontSize = 48);
    FontResource& loadFont(const std::string& name, const std::string& path, unsigned int fontSize = 48);
    FontResource& getFont(const std::string& name = "default");
    FontLoader& getFontLoader(const std::string& name = "default");
    bool reloadFont(const std::string& name, unsigned int newFontSize = 0);

    // --- Audio ---
    AudioResource& loadAudio(const std::string& name, const std::vector<std::string>& candidatePaths);
    AudioResource& loadAudio(const std::string& name, const std::string& path);
    AudioResource& getAudio(const std::string& name);
    bool reloadAudio(const std::string& name);

    // --- Script ---
    ScriptResource& loadScript(const std::string& name, const std::vector<std::string>& candidatePaths);
    ScriptResource& loadScript(const std::string& name, const std::string& path);
    ScriptResource& getScript(const std::string& name);
    bool reloadScript(const std::string& name);

private:
    struct FontConfig {
        std::vector<std::string> candidatePaths;
        unsigned int defaultSize{48};
    };

    std::string resolvePath(const std::vector<std::string>& candidates) const;

    LoggerSystem& m_logger;
    std::unordered_map<std::string, FontConfig> m_fontConfigs;
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<FontResource>> m_fonts;
    std::unordered_map<std::string, std::unique_ptr<AudioResource>> m_audios;
    std::unordered_map<std::string, std::unique_ptr<ScriptResource>> m_scripts;
};

#endif // GIRIGARA_RESOURCE_RESOURCE_MANAGER_H