#ifndef GIRIGARA_RESOURCE_SCRIPT_RESOURCE_H
#define GIRIGARA_RESOURCE_SCRIPT_RESOURCE_H

#include <string>
#include <Resource/IResource.h>

class ScriptResource : public IResource{
public:
    ScriptResource(std::string resolvedPath, std::string sourceCode)
    : m_path(std::move(resolvedPath)), m_code(std::move(sourceCode)) {}

    const std::string& getPath() const override { return m_path; }
    const std::string& getCode() const { return m_code; }

    bool reload() override;

private:
    std::string m_path;
    std::string m_code;
};

#endif// GIRIGARA_RESOURCE_SCRIPT_RESOURCE_H
