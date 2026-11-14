#include <Server/ServerResourceManager.h>
#include <System/LoggerSystem.h>
#include <fstream>
#include <sstream>
#include <algorithm>

ServerResourceManager::ServerResourceManager(LoggerSystem& logger)
: m_logger(logger) {}

ScriptResource& ServerResourceManager::loadScript(const std::string& name, const std::vector<std::string>& candidates) {
    auto& scriptMap = m_registry[std::type_index(typeid(ScriptResource))];
    auto it = scriptMap.find(name);
    if (it != scriptMap.end()) {
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

    std::unique_ptr<ScriptResource> res;

    // 1. 發行模式（preferArchive == true）：優先從加密 .luz 封包讀取
    if (m_preferArchive) {
        res = tryLoadFromArchive();
    }

    // 2. 開發模式（或封包未命中）：嘗試從本機硬碟路徑讀取
    if (!res) {
        std::string validPath = resolvePath(candidates);
        if (!validPath.empty()) {
            std::ifstream file(validPath);
            if (file.is_open()) {
                std::stringstream buf;
                buf << file.rdbuf();
                res = std::make_unique<ScriptResource>(validPath, buf.str());
            }
        }
    }

    // 3. Fallback：硬碟也找不到時，再從掛載的 .luz 封包補救
    if (!res && !m_preferArchive) {
        res = tryLoadFromArchive();
    }

    if (!res) {
        throw std::runtime_error("[ServerResourceManager] Script not found on disk or archive: " + name);
    }

    auto* rawPtr = res.get();
    scriptMap.emplace(name, std::move(res));
    return *rawPtr;
}

ScriptResource& ServerResourceManager::loadScript(const std::string& name, const std::string& path) {
    return loadScript(name, std::vector<std::string>{path});
}

IResource* ServerResourceManager::getInternal(std::type_index type, const std::string& name) {
    auto mapIt = m_registry.find(type);
    if (mapIt == m_registry.end()) return nullptr;

    auto resIt = mapIt->second.find(name);
    if (resIt == mapIt->second.end()) return nullptr;

    return resIt->second.get();
}

bool ServerResourceManager::hasInternal(std::type_index type, const std::string& name) const {
    auto mapIt = m_registry.find(type);
    if (mapIt == m_registry.end()) return false;
    return mapIt->second.find(name) != mapIt->second.end();
}

bool ServerResourceManager::reload(const std::type_index type, const std::string& name) {
    auto* res = getInternal(type, name);
    return res ? res->reload() : false;
}

// --- 封包掛載實作 ---
bool ServerResourceManager::mountArchive(const std::string& archivePath, const GpkArchive::Key256& key) {
    if (std::find(m_mountedPaths.begin(), m_mountedPaths.end(), archivePath) != m_mountedPaths.end()) {
        return true; // 已掛載過同路徑封包，直接跳過
    }

    auto archive = std::make_unique<GpkArchive>();
    if (!archive->open(archivePath, key)) {
        return false;
    }

    m_archives.push_back(std::move(archive));
    m_mountedPaths.push_back(archivePath);
    return true;
}

bool ServerResourceManager::extractFromArchives(const std::string& virtualPath, std::vector<uint8_t>& outBytes) const {
    // 逆向遍歷：後掛載的 Patch 封包優先覆蓋先掛載的基礎封包
    for (auto it = m_archives.rbegin(); it != m_archives.rend(); ++it) {
        if ((*it)->extractFile(virtualPath, outBytes)) {
            return true;
        }
    }
    return false;
}
