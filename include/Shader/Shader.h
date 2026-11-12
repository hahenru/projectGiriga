#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <Resource/IResource.h>
#include <string>

class Shader : public IResource {
public:
    // 從檔案路徑載入並編譯
    Shader(const std::string& vertexPath, const std::string& fragmentPath);

    ~Shader() override;

    // RAII：禁止拷貝，支援移動
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // IResource 介面實作
    const std::string& getPath() const override { return m_vertexPath; }
    bool reload() override;

    void use() const;

    GLuint getID() const { return m_programID; }

    // Uniform 輔助函式（依名稱設定）
    void setMat4(const std::string& name, const float* value) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    void setInt(const std::string& name, int value) const;

    // Uniform 輔助函式（依 Location 直接設定，支援 DSA）
    void setMat4(GLint location, const float* value) const;
    void setVec4(GLint location, float x, float y, float z, float w) const;
    void setInt(GLint location, int value) const;

private:
    GLuint m_programID = 0;
    std::string m_vertexPath;
    std::string m_fragmentPath;

    GLuint buildProgram(const std::string& vertexPath, const std::string& fragmentPath);
    std::string readFile(const std::string& filePath);
    GLuint compileShader(GLenum type, const char* source);
};

#endif // SHADER_H