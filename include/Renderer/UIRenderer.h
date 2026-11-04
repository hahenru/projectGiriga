#ifndef GIRIGARA_RENDERER_UI_RENDERER_H
#define GIRIGARA_RENDERER_UI_RENDERER_H

#include <Shader/Shader.h>
#include <Renderer/QuadMesh.h>
#include <vector>

struct ScissorRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

class UIRenderer {
public:
    UIRenderer(Shader& defaultShader, float screenWidth, float screenHeight);
    ~UIRenderer() = default;

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

    // 純色 Quad
	// 繪製基礎四邊形 (拆散的 RGBA)
    void drawQuad(float x, float y, float width, float height,
                  float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

    // ★ 支援 const float* 或 float color[4] 陣列傳參
    void drawQuad(float x, float y, float width, float height, const float* color) {
        if (!color) return;
        drawQuad(x, y, width, height, color[0], color[1], color[2], color[3]);
    }

    // ★ 供 UIText 使用的帶紋理 Quad 繪製介面
    void drawTexturedRect(float x, float y, float width, float height,
                          unsigned int textureID, const float color[4]);

    const Shader& getShader() const { return m_shader; }
    const float* getProjectionMatrix() const { return m_projection; }
    float getScreenWidth() const { return m_screenWidth; }
    float getScreenHeight() const { return m_screenHeight; }

private:
    void updateProjection();
    void applyScissor(const ScissorRect& rect);

    Shader& m_shader;
    QuadMesh m_quadMesh;
    std::vector<ScissorRect> m_scissorStack;
    float m_projection[16] = {};
    float m_screenWidth = 0.0f;
    float m_screenHeight = 0.0f;
};

#endif // GIRIGARA_RENDERER_UI_RENDERER_H