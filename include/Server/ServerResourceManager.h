#ifndef GIRIGARA_SERVER_SERVER_SERVERRESOURCEMANAGER_H
#define GIRIGARA_SERVER_SERVER_SERVERRESOURCEMANAGER_H

#include <Resource/IResourceManager.h>
#include <Resource/ScriptResource.h>
#include <Resource/GpkArchive.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

class LoggerSystem;

class ServerResourceManager : public IResourceManager {
public:
    explicit ServerResourceManager(LoggerSystem& logger);
    ~ServerResourceManager() override = default;

    // 專用腳本載入
    ScriptResource& loadScript(const std::string& name, const std::vector<std::string>& candidates);
    ScriptResource& loadScript(const std::string& name, const std::string& path);

    // --- 封包掛載 ---
    bool mountArchive(const std::string& archivePath, const GpkArchive::Key256& key);
    void setPreferArchive(bool prefer) { m_preferArchive = prefer; }
    bool isPreferArchive() const { return m_preferArchive; }

    bool reload(const std::type_index type, const std::string& name) override;

protected:
    IResource* getInternal(std::type_index type, const std::string& name) override;
    bool hasInternal(std::type_index type, const std::string& name) const override;

private:
    LoggerSystem& m_logger;
    // 依 type_index 聚合儲存資產指標
    std::unordered_map<std::type_index, std::unordered_map<std::string, std::unique_ptr<IResource>>> m_registry;

    // --- 封包掛載儲存池 ---
    bool extractFromArchives(const std::string& virtualPath, std::vector<uint8_t>& outBytes) const;
    std::vector<std::unique_ptr<GpkArchive>> m_archives;
    std::vector<std::string> m_mountedPaths;
    bool m_preferArchive{false};
};

#endif // GIRIGARA_SERVER_SERVER_SERVERRESOURCEMANAGER_H
