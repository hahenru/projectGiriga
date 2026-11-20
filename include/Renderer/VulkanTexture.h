#ifndef GIRIGARA_RENDERER_VULKAN_TEXTURE_H
#define GIRIGARA_RENDERER_VULKAN_TEXTURE_H

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdint>

class VulkanContext;

class VulkanTexture {
public:
    VulkanTexture() = default;
    // 從 CPU 記憶體像素上傳建立（channels = 1 用 VK_FORMAT_R8_UNORM，channels = 4 用 VK_FORMAT_R8G8B8A8_UNORM）
    VulkanTexture(VulkanContext& vkCtx, VkDescriptorPool descPool, VkDescriptorSetLayout descLayout,
                  const void* pixels, int width, int height, int channels = 4);
    // 包裝外部離屏渲染目標的 VkImageView（不擁有 VkImage，僅建立 Sampler 與 DescriptorSet，供 UIViewport 使用）
    VulkanTexture(VulkanContext& vkCtx, VkDescriptorPool descPool, VkDescriptorSetLayout descLayout,
                  VkImageView externalView, int width, int height);
    ~VulkanTexture();

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;
    VulkanTexture(VulkanTexture&& other) noexcept;
    VulkanTexture& operator=(VulkanTexture&& other) noexcept;

    VkDescriptorSet getDescriptorSet() const { return m_descriptorSet; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    bool isValid() const { return m_imageView != VK_NULL_HANDLE; }

private:
    void createSamplerAndDescriptor(VkDescriptorPool descPool, VkDescriptorSetLayout descLayout);
    void release();

    VulkanContext* m_vkCtx = nullptr;
    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    int m_width = 0;
    int m_height = 0;
    bool m_ownsImage = false;
};

#endif // GIRIGARA_RENDERER_VULKAN_TEXTURE_H