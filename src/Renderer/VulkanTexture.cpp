#include <Renderer/VulkanTexture.h>
#include <Renderer/VulkanContext.h>
#include <cstring>
#include <stdexcept>
#include <utility>

VulkanTexture::VulkanTexture(VulkanContext& vkCtx, VkDescriptorPool descPool, VkDescriptorSetLayout descLayout,
                             const void* pixels, int width, int height, int channels)
    : m_vkCtx(&vkCtx), m_width(width), m_height(height), m_ownsImage(true) {

    if (!pixels || width <= 0 || height <= 0) return;

    const VkFormat format = (channels == 1) ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * channels;

    // 1. 建立 CPU 可寫入的 Staging Buffer
    VkBufferCreateInfo stagingBufInfo{};
    stagingBufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufInfo.size = imageSize;
    stagingBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAlloc = VK_NULL_HANDLE;
    if (vmaCreateBuffer(m_vkCtx->getAllocator(), &stagingBufInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAlloc, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("[VulkanTexture] Failed to create staging buffer");
    }

    void* mappedData = nullptr;
    vmaMapMemory(m_vkCtx->getAllocator(), stagingAlloc, &mappedData);
    std::memcpy(mappedData, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(m_vkCtx->getAllocator(), stagingAlloc);

    // 2. 建立 GPU 端 VkImage
    VkImageCreateInfo imgInfo{};
    imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.format = format;
    imgInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imgInfo.mipLevels = 1;
    imgInfo.arrayLayers = 1;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imgAllocInfo{};
    imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(m_vkCtx->getAllocator(), &imgInfo, &imgAllocInfo,
                       &m_image, &m_allocation, nullptr) != VK_SUCCESS) {
        vmaDestroyBuffer(m_vkCtx->getAllocator(), stagingBuffer, stagingAlloc);
        throw std::runtime_error("[VulkanTexture] Failed to create GPU VkImage");
    }

    // 3. 透過單次指令提交拷貝至 GPU 並轉換 Layout 為 SHADER_READ_ONLY_OPTIMAL
    m_vkCtx->immediateSubmit([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier2 toTransfer{};
        toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        toTransfer.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        toTransfer.srcAccessMask = 0;
        toTransfer.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        toTransfer.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer.image = m_image;
        toTransfer.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        VkDependencyInfo dep1{};
        dep1.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep1.imageMemoryBarrierCount = 1;
        dep1.pImageMemoryBarriers = &toTransfer;
        vkCmdPipelineBarrier2(cmd, &dep1);

        VkBufferImageCopy copyRegion{};
        copyRegion.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
        vkCmdCopyBufferToImage(cmd, stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        VkImageMemoryBarrier2 toShaderRead{};
        toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        toShaderRead.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        toShaderRead.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        toShaderRead.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        toShaderRead.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        toShaderRead.image = m_image;
        toShaderRead.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        VkDependencyInfo dep2{};
        dep2.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep2.imageMemoryBarrierCount = 1;
        dep2.pImageMemoryBarriers = &toShaderRead;
        vkCmdPipelineBarrier2(cmd, &dep2);
    });

    vmaDestroyBuffer(m_vkCtx->getAllocator(), stagingBuffer, stagingAlloc);

    // 4. 建立 VkImageView 與 DescriptorSet
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    if (vkCreateImageView(m_vkCtx->getDevice(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        release();
        throw std::runtime_error("[VulkanTexture] Failed to create VkImageView");
    }

    createSamplerAndDescriptor(descPool, descLayout);
}

VulkanTexture::VulkanTexture(VulkanContext& vkCtx, VkDescriptorPool descPool, VkDescriptorSetLayout descLayout,
                             VkImageView externalView, int width, int height)
    : m_vkCtx(&vkCtx), m_imageView(externalView), m_width(width), m_height(height), m_ownsImage(false) {

    if (externalView == VK_NULL_HANDLE) return;
    createSamplerAndDescriptor(descPool, descLayout);
}

VulkanTexture::~VulkanTexture() {
    release();
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept
    : m_vkCtx(other.m_vkCtx),
      m_image(other.m_image),
      m_allocation(other.m_allocation),
      m_imageView(other.m_imageView),
      m_sampler(other.m_sampler),
      m_descriptorSet(other.m_descriptorSet),
      m_width(other.m_width),
      m_height(other.m_height),
      m_ownsImage(other.m_ownsImage) {
    other.m_vkCtx = nullptr;
    other.m_image = VK_NULL_HANDLE;
    other.m_allocation = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_sampler = VK_NULL_HANDLE;
    other.m_descriptorSet = VK_NULL_HANDLE;
    other.m_width = 0;
    other.m_height = 0;
    other.m_ownsImage = false;
}

VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept {
    if (this != &other) {
        release();
        m_vkCtx = other.m_vkCtx;
        m_image = other.m_image;
        m_allocation = other.m_allocation;
        m_imageView = other.m_imageView;
        m_sampler = other.m_sampler;
        m_descriptorSet = other.m_descriptorSet;
        m_width = other.m_width;
        m_height = other.m_height;
        m_ownsImage = other.m_ownsImage;

        other.m_vkCtx = nullptr;
        other.m_image = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_sampler = VK_NULL_HANDLE;
        other.m_descriptorSet = VK_NULL_HANDLE;
        other.m_width = 0;
        other.m_height = 0;
        other.m_ownsImage = false;
    }
    return *this;
}

void VulkanTexture::createSamplerAndDescriptor(VkDescriptorPool descPool, VkDescriptorSetLayout descLayout) {
    VkDevice dev = m_vkCtx->getDevice();

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(dev, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        release();
        throw std::runtime_error("[VulkanTexture] Failed to create VkSampler");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descLayout;

    if (vkAllocateDescriptorSets(dev, &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
        release();
        throw std::runtime_error("[VulkanTexture] Failed to allocate VkDescriptorSet");
    }

    VkDescriptorImageInfo imgDescInfo{};
    imgDescInfo.sampler = m_sampler;
    imgDescInfo.imageView = m_imageView;
    imgDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_descriptorSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imgDescInfo;

    vkUpdateDescriptorSets(dev, 1, &write, 0, nullptr);
}

void VulkanTexture::release() {
    if (!m_vkCtx) return;
    VkDevice dev = m_vkCtx->getDevice();

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(dev, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    if (m_ownsImage) {
        if (m_imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(dev, m_imageView, nullptr);
            m_imageView = VK_NULL_HANDLE;
        }
        if (m_image != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
            vmaDestroyImage(m_vkCtx->getAllocator(), m_image, m_allocation);
            m_image = VK_NULL_HANDLE;
            m_allocation = VK_NULL_HANDLE;
        }
    }

    m_descriptorSet = VK_NULL_HANDLE;
    m_vkCtx = nullptr;
}