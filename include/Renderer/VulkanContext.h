#ifndef GIRIGARA_RENDERER_VULKAN_CONTEXT_H
#define GIRIGARA_RENDERER_VULKAN_CONTEXT_H

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <functional>
#include <cstdint>

class LoggerSystem;

class VulkanContext {
public:
    explicit VulkanContext(LoggerSystem& logger, bool enableValidation = true);
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    VkInstance getInstance() const { return m_vkbInstance.instance; }
    VkPhysicalDevice getPhysicalDevice() const { return m_vkbPhysicalDevice.physical_device; }
    VkDevice getDevice() const { return m_vkbDevice.device; }
    VmaAllocator getAllocator() const { return m_allocator; }

    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    uint32_t getGraphicsQueueFamily() const { return m_graphicsQueueFamily; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    uint32_t getPresentQueueFamily() const { return m_presentQueueFamily; }

    // 供資源上傳（字形/貼圖 Staging Buffer）使用的單次同步指令輔助
    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& action) const;
    void waitIdle() const;

private:
    LoggerSystem& m_logger;

    vkb::Instance m_vkbInstance;
    vkb::PhysicalDevice m_vkbPhysicalDevice;
    vkb::Device m_vkbDevice;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamily = 0;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    uint32_t m_presentQueueFamily = 0;

    VmaAllocator m_allocator = VK_NULL_HANDLE;

    VkCommandPool m_uploadCmdPool = VK_NULL_HANDLE;
    VkFence m_uploadFence = VK_NULL_HANDLE;
};

#endif // GIRIGARA_RENDERER_VULKAN_CONTEXT_H