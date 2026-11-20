#ifndef GIRIGARA_RENDERER_VULKAN_RENDER_TARGET_H
#define GIRIGARA_RENDERER_VULKAN_RENDER_TARGET_H

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <Renderer/VulkanTexture.h>
#include <cstdint>

class VulkanContext;

class VulkanRenderTarget {
public:
    VulkanRenderTarget(VulkanContext& vkCtx, VkDescriptorPool descPool, VkDescriptorSetLayout descLayout,
                       int width, int height, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM);
    ~VulkanRenderTarget();

    VulkanRenderTarget(const VulkanRenderTarget&) = delete;
    VulkanRenderTarget& operator=(const VulkanRenderTarget&) = delete;

    // 当 UIWindow 或 UIViewport 縮放時安全重建離屏影像
    void resize(int width, int height);

    // Pass 1：開啟離屏 Dynamic Rendering 並清屏（供目標 Scene 錄製繪圖指令）
    void beginRendering(VkCommandBuffer cmd, float clearR = 0.08f, float clearG = 0.10f, float clearB = 0.14f);
    // 結束離屏渲染，並自動插入 Memory Barrier 轉為 SHADER_READ_ONLY_OPTIMAL
    void endRendering(VkCommandBuffer cmd);

    VulkanTexture& getTexture() { return m_texture; }
    const VulkanTexture& getTexture() const { return m_texture; }
    VkFormat getFormat() const { return m_format; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    void createResources();
    void destroyResources();

    VulkanContext& m_vkCtx;
    VkDescriptorPool m_descPool;
    VkDescriptorSetLayout m_descLayout;

    int m_width = 0;
    int m_height = 0;
    VkFormat m_format;

    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VulkanTexture m_texture;
};

#endif // GIRIGARA_RENDERER_VULKAN_RENDER_TARGET_H