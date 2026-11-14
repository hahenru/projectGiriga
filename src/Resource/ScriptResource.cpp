#include <Resource/ScriptResource.h>
#include <fstream>
#include <sstream>
#include <iostream>

bool ScriptResource::reload() {
    std::ifstream file(m_path);
    if (!file.is_open()) {
        std::cerr << "[ScriptResource] Failed to reload script from: " << m_path << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    m_code = buffer.str();
    return true;
}
