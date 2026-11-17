#ifndef GIRIGARA_TOOL_TOOL_RESOURCE_MANAGER_H
#define GIRIGARA_TOOL_TOOL_RESOURCE_MANAGER_H

#include <Resource/IResourceManager.h>
#include <Shader/Shader.h>
#include <Resource/FontResource.h>
#include <Resource/ScriptResource.h>
#include <Resource/ImageResource.h>
#include <Resource/GpkArchive.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

class LoggerSystem;

class ToolResourceManager : public IResourceManager {
public:
    explicit ToolResourceManager(LoggerSystem& logger);
    ~ToolResourceManager() override = default;

    // --- Shader ---
    Shader& loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    Shader& getShader(const std::string& name);
	Shader& getOrLoadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath) override;

    // --- Font ---
    void registerFontPath(const std::string& name, const std::vector<std::string>& candidates, unsigned int defaultSize = 48);
    FontResource& loadFont(const std::string& name, const std::vector<std::string>& candidates, unsigned int fontSize = 48);
    FontResource& loadFont(const std::string& name, const std::string& path, unsigned int fontSize = 48);
    FontResource& getFont(const std::string& name = "default");
    FontLoader& getFontLoader(const std::string& name = "default");

    // --- Script（供 LuaScene 載入工具腳本）---
    ScriptResource& loadScript(const std::string& name, const std::vector<std::string>& candidates);
    ScriptResource& loadScript(const std::string& name, const std::string& path);
    ScriptResource& getScript(const std::string& name);

    // --- Image（供工具預覽圖資）---
    ImageResource& loadImage(const std::string& name, const std::vector<std::string>& candidates, bool flipVertically = true);
    ImageResource& loadImage(const std::string& name, const std::string& path, bool flipVertically = true);
    ImageResource& getImage(const std::string& name);

    // --- GpkArchive 封包掛載 ---
    bool mountArchive(const std::string& archivePath, const GpkArchive::Key256& key);
    void setPreferArchive(bool prefer) { m_preferArchive = prefer; }
    bool isPreferArchive() const { return m_preferArchive; }

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

    bool extractFromArchives(const std::string& virtualPath, std::vector<uint8_t>& outBytes) const;
    std::vector<std::unique_ptr<GpkArchive>> m_archives;
    std::vector<std::string> m_mountedPaths;
    bool m_preferArchive{false};
};

#endif // GIRIGARA_TOOL_TOOL_RESOURCE_MANAGER_H
