#ifndef GIRIGARA_SERVER_SERVER_SERVERRESOURCEMANAGER_H
#define GIRIGARA_SERVER_SERVER_SERVERRESOURCEMANAGER_H
#include <Resource/IResourceManager.h>
#include <Resource/ScriptResource.h>
#include <unordered_map>
#include <memory>

class LoggerSystem;

class ServerResourceManager : public IResourceManager {
public:
    explicit ServerResourceManager(LoggerSystem& logger);
    ~ServerResourceManager() override = default;

    // 專用腳本載入
    ScriptResource& loadScript(const std::string& name, const std::vector<std::string>& candidates);
    ScriptResource& loadScript(const std::string& name, const std::string& path);

    bool reload(const std::type_index type, const std::string& name) override;

protected:
    IResource* getInternal(std::type_index type, const std::string& name) override;
    bool hasInternal(std::type_index type, const std::string& name) const override;

private:
    LoggerSystem& m_logger;
    // 依 type_index 聚合儲存資產指標
    std::unordered_map<std::type_index, std::unordered_map<std::string, std::unique_ptr<IResource>>> m_registry;
};

#endif// GIRIGARA_SERVER_SERVER_SERVERRESOURCEMANAGER_H