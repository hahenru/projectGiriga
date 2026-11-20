#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <Renderer/VulkanContext.h>
#include <System/LoggerSystem.h>
#include <stdexcept>

VulkanContext::VulkanContext(LoggerSystem& logger, bool enableValidation)
    : m_logger(logger) {

    // 1. 建立 VkInstance（最低要求 Vulkan 1.3，相容 Intel Iris Xe / AMD RX 6700 XT / Android）
    vkb::InstanceBuilder instBuilder;
    auto instRet = instBuilder
        .set_app_name("Project Girigara")
        .set_engine_name("Girigara Engine")
        .require_api_version(1, 3, 0)
        .request_validation_layers(enableValidation)
        .use_default_debug_messenger()
        .build();

    if (!instRet) {
        throw std::runtime_error("[VulkanContext] Failed to create VkInstance: " + instRet.error().message());
    }
    m_vkbInstance = instRet.value();

    // 2. 挑選實體 GPU（啟用 Vulkan 1.3 Dynamic Rendering 與 Synchronization2）
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    vkb::PhysicalDeviceSelector physSelector{ m_vkbInstance };
    auto physRet = physSelector
        .set_minimum_version(1, 3)
        .set_required_features_13(features13)
        .defer_surface_initialization()
        .select();

    if (!physRet) {
        vkb::destroy_instance(m_vkbInstance);
        throw std::runtime_error("[VulkanContext] Failed to select PhysicalDevice: " + physRet.error().message());
    }
    m_vkbPhysicalDevice = physRet.value();
    m_logger.info(std::string("[VulkanContext] 選用 GPU: ") + m_vkbPhysicalDevice.name);

    // 3. 建立邏輯裝置 VkDevice
    vkb::DeviceBuilder devBuilder{ m_vkbPhysicalDevice };
    auto devRet = devBuilder.build();
    if (!devRet) {
        vkb::destroy_instance(m_vkbInstance);
        throw std::runtime_error("[VulkanContext] Failed to create VkDevice: " + devRet.error().message());
    }
    m_vkbDevice = devRet.value();

    auto gfxQueueRet = m_vkbDevice.get_queue(vkb::QueueType::graphics);
    auto gfxFamilyRet = m_vkbDevice.get_queue_index(vkb::QueueType::graphics);
    if (!gfxQueueRet || !gfxFamilyRet) {
        vkb::destroy_device(m_vkbDevice);
        vkb::destroy_instance(m_vkbInstance);
        throw std::runtime_error("[VulkanContext] Failed to get Vulkan Graphics Queue");
    }

    m_graphicsQueue = gfxQueueRet.value();
    m_graphicsQueueFamily = gfxFamilyRet.value();
    m_presentQueue = m_graphicsQueue;
    m_presentQueueFamily = m_graphicsQueueFamily;

    // 4. 初始化 VMA 記憶體配置器
    VmaAllocatorCreateInfo allocInfo{};
    allocInfo.physicalDevice = m_vkbPhysicalDevice.physical_device;
    allocInfo.device = m_vkbDevice.device;
    allocInfo.instance = m_vkbInstance.instance;
    allocInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    if (vmaCreateAllocator(&allocInfo, &m_allocator) != VK_SUCCESS) {
        vkb::destroy_device(m_vkbDevice);
        vkb::destroy_instance(m_vkbInstance);
        throw std::runtime_error("[VulkanContext] Failed to create VMA allocator");
    }

    // 5. 建立資源上傳專用的短指令池與圍欄
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamily;
    vkCreateCommandPool(m_vkbDevice.device, &poolInfo, nullptr, &m_uploadCmdPool);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_vkbDevice.device, &fenceInfo, nullptr, &m_uploadFence);
}

VulkanContext::~VulkanContext() {
    if (m_vkbDevice.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_vkbDevice.device);
        if (m_uploadFence != VK_NULL_HANDLE) vkDestroyFence(m_vkbDevice.device, m_uploadFence, nullptr);
        if (m_uploadCmdPool != VK_NULL_HANDLE) vkDestroyCommandPool(m_vkbDevice.device, m_uploadCmdPool, nullptr);
    }
    if (m_allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(m_allocator);
    }
    vkb::destroy_device(m_vkbDevice);
    vkb::destroy_instance(m_vkbInstance);
}

void VulkanContext::waitIdle() const {
    if (m_vkbDevice.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_vkbDevice.device);
    }
}

void VulkanContext::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& action) const {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_uploadCmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(m_vkbDevice.device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    action(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_uploadFence);
    vkWaitForFences(m_vkbDevice.device, 1, &m_uploadFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_vkbDevice.device, 1, &m_uploadFence);

    vkFreeCommandBuffers(m_vkbDevice.device, m_uploadCmdPool, 1, &cmd);
}
