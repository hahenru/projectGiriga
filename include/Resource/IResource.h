#ifndef GIRIGARA_INCLUDE_RESOURCE_IRESOURCE_H
#define GIRIGARA_INCLUDE_RESOURCE_IRESOURCE_H
#include <string>

class IResource {
public:
    virtual ~IResource() = default;
    virtual const std::string& getPath() const = 0;
    virtual bool reload() = 0;
};

#endif /* GIRIGARA_INCLUDE_RESOURCE_IRESOURCE_H */