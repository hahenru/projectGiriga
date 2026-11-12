#ifndef GIRIGARA_INCLUDE_RESOURCE_IRESOURCEMANAGER_H
#define GIRIGARA_INCLUDE_RESOURCE_IRESOURCEMANAGER_H

#include <string>
#include <vector>
#include <typeindex>
#include <stdexcept>
#include <filesystem>
#include <Resource/IResource.h>

class IResourceManager {
public:
    virtual ~IResourceManager() = default;

    template<typename T>
    T& get(const std::string& name) {
        auto* res = getInternal(std::type_index(typeid(T)), name);
        if (!res) {
            throw std::runtime_error("[IResourceManager] Resource not found: " + name);
        }
        return *static_cast<T*>(res);
    }

    template<typename T>
    bool has(const std::string& name) const {
        return hasInternal(std::type_index(typeid(T)), name);
    }

    // 統一使用 std::type_index 傳值
    virtual bool reload(std::type_index type, const std::string& name) = 0;

    static std::string resolvePath(const std::vector<std::string>& candidates) {
        for (const auto& path : candidates) {
            if (std::filesystem::exists(path)) {
                return path;
            }
        }
        return "";
    }

protected:
    virtual IResource* getInternal(std::type_index type, const std::string& name) = 0;
    virtual bool hasInternal(std::type_index type, const std::string& name) const = 0;
};

#endif // GIRIGARA_INCLUDE_RESOURCE_IRESOURCEMANAGER_H