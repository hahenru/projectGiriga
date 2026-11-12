#include <Server/ServerResourceManager.h>
#include <System/LoggerSystem.h>
#include <fstream>
#include <sstream>

ServerResourceManager::ServerResourceManager(LoggerSystem& logger)
    : m_logger(logger) {}

ScriptResource& ServerResourceManager::loadScript(const std::string& name, const std::vector<std::string>& candidates) {
    auto& scriptMap = m_registry[std::type_index(typeid(ScriptResource))];
    auto it = scriptMap.find(name);
    if (it != scriptMap.end()) {
        return *static_cast<ScriptResource*>(it->second.get());
    }

    std::string validPath = resolvePath(candidates);
    if (validPath.empty()) {
        throw std::runtime_error("[ServerResourceManager] Script path not found: " + name);
    }

    std::ifstream file(validPath);
    if (!file.is_open()) {
        throw std::runtime_error("[ServerResourceManager] Cannot open file: " + validPath);
    }

    std::stringstream buf;
    buf << file.rdbuf();

    auto res = std::make_unique<ScriptResource>(validPath, buf.str());
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
    auto* res = getInternal(std::type_index(type), name);
    return res ? res->reload() : false;
}