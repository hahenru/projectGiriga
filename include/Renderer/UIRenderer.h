#ifndef GIRIGARA_RENDERER_UI_RENDERER_H
#define GIRIGARA_RENDERER_UI_RENDERER_H

#include <Shader/Shader.h>
#include <Renderer/QuadMesh.h>
#include <Resource/ImageResource.h>
#include <vector>
#include <unordered_map>
#include <cstdint>

struct ScissorRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

class UIRenderer {
public:
    UIRenderer(Shader& defaultShader, float screenWidth, float screenHeight);
    ~UIRenderer();

    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;
    UIRenderer(UIRenderer&&) noexcept = default;
    UIRenderer& operator=(UIRenderer&&) noexcept = default;

    void setScreenSize(float width, float height);

    void begin();
    void beginCurrent();
    void end();

    void pushClipRect(float x, float y, float width, float height);
    void popClipRect();

    // 1. 純色 Quad
    void drawQuad(float x, float y, float width, float height,
                  float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

    void drawQuad(float x, float y, float width, float height, const float* color) {
        if (!color) return;
        drawQuad(x, y, width, height, color[0], color[1], color[2], color[3]);
    }

    // 2. 供 UIText 使用的單通道字體貼圖 Quad
    void drawTexturedRect(float x, float y, float width, float height,
                          unsigned int textureID, const float color[4]);

    // 3. 供 UI 控制項繪製一般 RGBA 圖片（自動在主執行緒懶上傳並追蹤熱重載）
    void drawImage(float x, float y, float width, float height,
                   const ImageResource& image,
                   float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

    // 4. 供 UIConsole 等特殊動態特效使用的自訂 Shader Quad
    void drawShaderQuad(Shader& customShader, float x, float y, float width, float height, float elapsedTime);

    const Shader& getShader() const { return m_shader; }
    const float* getProjectionMatrix() const { return m_projection; }
    float getScreenWidth() const { return m_screenWidth; }
    float getScreenHeight() const { return m_screenHeight; }

private:
    struct CachedTexture {
        unsigned int textureID = 0;
        uint64_t uploadedRevision = 0;
    };

    unsigned int getOrUploadTexture(const ImageResource& image);
    void updateProjection();
    void applyScissor(const ScissorRect& rect);

    Shader& m_shader;
    QuadMesh m_quadMesh;
    std::vector<ScissorRect> m_scissorStack;
    std::unordered_map<const ImageResource*, CachedTexture> m_textureCache;
    float m_projection[16] = {};
    float m_screenWidth = 0.0f;
    float m_screenHeight = 0.0f;
};

#endif // GIRIGARA_RENDERER_UI_RENDERER_H
