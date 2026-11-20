#include <Renderer/VulkanRenderTarget.h>
#include <Renderer/VulkanContext.h>
#include <stdexcept>
#include <algorithm>

VulkanRenderTarget::VulkanRenderTarget(VulkanContext& vkCtx, VkDescriptorPool descPool, VkDescriptorSetLayout descLayout,
                                       int width, int height, VkFormat format)
    : m_vkCtx(vkCtx), m_descPool(descPool), m_descLayout(descLayout),
      m_width(std::max(1, width)), m_height(std::max(1, height)), m_format(format) {
    createResources();
}

VulkanRenderTarget::~VulkanRenderTarget() {
    destroyResources();
}

void VulkanRenderTarget::resize(int width, int height) {
    int safeW = std::max(1, width);
    int safeH = std::max(1, height);
    if (safeW == m_width && safeH == m_height) return;

    m_vkCtx.waitIdle();
    destroyResources();
    m_width = safeW;
    m_height = safeH;
    createResources();
}

void VulkanRenderTarget::createResources() {
    VkImageCreateInfo imgInfo{};
    imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.format = m_format;
    imgInfo.extent = { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height), 1 };
    imgInfo.mipLevels = 1;
    imgInfo.arrayLayers = 1;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(m_vkCtx.getAllocator(), &imgInfo, &allocInfo,
                       &m_image, &m_allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("[VulkanRenderTarget] Failed to create offscreen VkImage");
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_format;
    viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    if (vkCreateImageView(m_vkCtx.getDevice(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        destroyResources();
        throw std::runtime_error("[VulkanRenderTarget] Failed to create offscreen VkImageView");
    }

    // 包裝外部 ImageView 產生可供 UIRenderer 採樣的 DescriptorSet
    m_texture = VulkanTexture(m_vkCtx, m_descPool, m_descLayout, m_imageView, m_width, m_height);
}

void VulkanRenderTarget::destroyResources() {
    m_texture = VulkanTexture(); // 先銷毀 Sampler 與 DescriptorSet

    VkDevice dev = m_vkCtx.getDevice();
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(dev, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }
    if (m_image != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
        vmaDestroyImage(m_vkCtx.getAllocator(), m_image, m_allocation);
        m_image = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }
}

void VulkanRenderTarget::beginRendering(VkCommandBuffer cmd, float clearR, float clearG, float clearB) {
    // 轉換 Layout: UNDEFINED / SHADER_READ_ONLY_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.image = m_image;
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd, &depInfo);

    VkRenderingAttachmentInfo colorAtt{};
    colorAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAtt.imageView = m_imageView;
    colorAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.clearValue.color = {{ clearR, clearG, clearB, 1.0f }};

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.renderArea = { {0, 0}, { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) } };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAtt;

    vkCmdBeginRendering(cmd, &renderInfo);

    VkViewport vp{ 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
    VkRect2D scissor{ {0, 0}, { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) } };
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void VulkanRenderTarget::endRendering(VkCommandBuffer cmd) {
    vkCmdEndRendering(cmd);

    // 轉換 Layout: COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL（供緊接著的 UIWindow 貼圖採樣）
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.image = m_image;
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd, &depInfo);
}