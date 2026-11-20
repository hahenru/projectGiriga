#ifndef GIRIGARA_RENDERER_UI_RENDERER_H
#define GIRIGARA_RENDERER_UI_RENDERER_H

#include <vulkan/vulkan.h>
#include <Font/FontLoader.h>
#include <glm/glm.hpp>
#include <Renderer/QuadMesh.h>
#include <Renderer/VulkanTexture.h>
#include <Resource/ImageResource.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>

class VulkanContext;

struct ScissorRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

// 與 ui.vert / ui.frag / console_bg.frag 完全對齊的 48-byte Push Constants
struct UIPushConstants {
    glm::vec4 rect;   // xy = 左上角像素座標, zw = 寬高 (w, h)
    glm::vec4 color;  // rgba
    glm::vec4 params; // x = renderMode (0=純色, 1=字體單通道R, 2=RGBA貼圖), y = time, zw = screenSize
};

class UIRenderer {
public:
    UIRenderer(VulkanContext& vkCtx, VkFormat colorFormat, float screenWidth, float screenHeight);
    ~UIRenderer();

    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;

    void setScreenSize(float width, float height);

    // 綁定本幀錄製目標 CommandBuffer 與 Pipeline
    void begin(VkCommandBuffer cmd);
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

    // 2. 直接繪製原生 VulkanTexture（供 UIText、UIViewport 離屏視口使用）
    void drawTexture(float x, float y, float width, float height,
                     const VulkanTexture& texture,
                     float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f,
                     int renderMode = 2);

    // 3. 供 UI 控制項繪製一般 RGBA 圖片（自動在主執行緒懶上傳並追蹤熱重載）
    void drawImage(float x, float y, float width, float height,
                   const ImageResource& image,
                   float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

    // 4. 供 UIConsole 等特殊動態特效使用的自訂管線 Quad
    void drawConsoleBgQuad(float x, float y, float width, float height, float elapsedTime);

	// 5. 供 UIText 繪製單通道字形點陣（自動懶上傳並依 revision 更新快取）
    void drawText(float x, float y, float width, float height,
                  const TextTexture& textTex, const float color[4]);
				  
    VulkanContext& getVulkanContext() { return m_vkCtx; }
    VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }
    VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }
    float getScreenWidth() const { return m_screenWidth; }
    float getScreenHeight() const { return m_screenHeight; }

private:
    struct CachedTexture {
        VulkanTexture texture;
        uint64_t uploadedRevision = 0;
    };

    void createDescriptorResources();
    void createPipelines(VkFormat colorFormat);
    VkShaderModule loadShaderModule(const std::string& spvPath);
    VkPipeline buildGraphicsPipeline(VkShaderModule vertModule, VkShaderModule fragModule, VkFormat colorFormat);

    const VulkanTexture* getOrUploadTexture(const ImageResource& image);
    void applyScissor(const ScissorRect& rect);
    void resetFullScreenScissor();

    VulkanContext& m_vkCtx;
    QuadMesh m_quadMesh;

    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_uiPipeline = VK_NULL_HANDLE;
    VkPipeline m_consoleBgPipeline = VK_NULL_HANDLE;

    VulkanTexture m_whiteTexture;
    VkCommandBuffer m_currentCmd = VK_NULL_HANDLE;

    std::vector<ScissorRect> m_scissorStack;
    std::unordered_map<const ImageResource*, CachedTexture> m_textureCache;
    float m_screenWidth = 0.0f;
    float m_screenHeight = 0.0f;
	std::unordered_map<const TextTexture*, CachedTexture> m_textCache;
};

#endif // GIRIGARA_RENDERER_UI_RENDERER_H