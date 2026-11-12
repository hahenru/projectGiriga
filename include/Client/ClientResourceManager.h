#ifndef GIRIGARA_CLIENT_CLIENT_CLIENTRESOURCEMANAGER_H
#define GIRIGARA_CLIENT_CLIENT_CLIENTRESOURCEMANAGER_H
#include <Resource/IResourceManager.h>
#include <Shader/Shader.h>
#include <Resource/FontResource.h>
#include <Resource/AudioResource.h>
#include <Resource/ScriptResource.h>
#include <unordered_map>
#include <memory>

class LoggerSystem;

class ClientResourceManager : public IResourceManager {
public:
    explicit ClientResourceManager(LoggerSystem& logger);
    ~ClientResourceManager() override = default;

    // --- Shader ---
    Shader& loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    Shader& getShader(const std::string& name);

    // --- Font ---
    void registerFontPath(const std::string& name, const std::vector<std::string>& candidates, unsigned int defaultSize = 48);
    FontResource& loadFont(const std::string& name, const std::vector<std::string>& candidates, unsigned int fontSize = 48);
    FontResource& loadFont(const std::string& name, const std::string& path, unsigned int fontSize = 48);
    FontResource& getFont(const std::string& name = "default");
    FontLoader& getFontLoader(const std::string& name = "default");

    // --- Audio ---
    AudioResource& loadAudio(const std::string& name, const std::vector<std::string>& candidates);
    AudioResource& loadAudio(const std::string& name, const std::string& path);
    AudioResource& getAudio(const std::string& name);

    // --- Script ---
    ScriptResource& loadScript(const std::string& name, const std::vector<std::string>& candidates);
    ScriptResource& loadScript(const std::string& name, const std::string& path);
    ScriptResource& getScript(const std::string& name);

    bool reload(const std::type_index type, const std::string& name) override;

protected:
    IResource* getInternal(std::type_index type, const std::string& name) override;
    bool hasInternal(std::type_index type, const std::string& name) const override;

private:
    struct FontConfig {
        std::vector<std::string> candidatePaths;
        unsigned int defaultSize{48};
    };

    LoggerSystem& m_logger;
    std::unordered_map<std::string, FontConfig> m_fontConfigs;
    std::unordered_map<std::type_index, std::unordered_map<std::string, std::unique_ptr<IResource>>> m_registry;
};

#endif// GIRIGARA_CLIENT_CLIENT_CLIENTRESOURCEMANAGER_H