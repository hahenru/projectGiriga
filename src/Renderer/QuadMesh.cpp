#include <Renderer/QuadMesh.h>
#include <Renderer/VulkanContext.h>
#include <cstring>
#include <stdexcept>

QuadMesh::QuadMesh(VulkanContext& vkCtx) : m_vkCtx(&vkCtx) {
    // 單位 Quad：2 個直角三角形 (x, y, u, v)，左上 (0,0) 至右下 (1,1)
    const float vertices[] = {
        0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 0.0f,   1.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 1.0f,

        0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 1.0f,
        0.0f, 1.0f,   0.0f, 1.0f
    };

    const VkDeviceSize bufferSize = sizeof(vertices);

    // 1. 建立 CPU 可寫入 Staging Buffer
    VkBufferCreateInfo stagingInfo{};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = bufferSize;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    VkBuffer stagingBuf = VK_NULL_HANDLE;
    VmaAllocation stagingAlloc = VK_NULL_HANDLE;
    if (vmaCreateBuffer(m_vkCtx->getAllocator(), &stagingInfo, &stagingAllocInfo,
                        &stagingBuf, &stagingAlloc, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("[QuadMesh] Failed to create staging buffer");
    }

    void* mapped = nullptr;
    vmaMapMemory(m_vkCtx->getAllocator(), stagingAlloc, &mapped);
    std::memcpy(mapped, vertices, static_cast<size_t>(bufferSize));
    vmaUnmapMemory(m_vkCtx->getAllocator(), stagingAlloc);

    // 2. 建立 GPU 端 Vertex Buffer
    VkBufferCreateInfo vbInfo{};
    vbInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vbInfo.size = bufferSize;
    vbInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo vbAllocInfo{};
    vbAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateBuffer(m_vkCtx->getAllocator(), &vbInfo, &vbAllocInfo,
                        &m_vertexBuffer, &m_allocation, nullptr) != VK_SUCCESS) {
        vmaDestroyBuffer(m_vkCtx->getAllocator(), stagingBuf, stagingAlloc);
        throw std::runtime_error("[QuadMesh] Failed to create GPU vertex buffer");
    }

    m_vkCtx->immediateSubmit([&](VkCommandBuffer cmd) {
        VkBufferCopy copyRegion{ 0, 0, bufferSize };
        vkCmdCopyBuffer(cmd, stagingBuf, m_vertexBuffer, 1, &copyRegion);
    });

    vmaDestroyBuffer(m_vkCtx->getAllocator(), stagingBuf, stagingAlloc);
}

QuadMesh::~QuadMesh() {
    release();
}

QuadMesh::QuadMesh(QuadMesh&& other) noexcept
    : m_vkCtx(other.m_vkCtx), m_vertexBuffer(other.m_vertexBuffer), m_allocation(other.m_allocation) {
    other.m_vkCtx = nullptr;
    other.m_vertexBuffer = VK_NULL_HANDLE;
    other.m_allocation = VK_NULL_HANDLE;
}

QuadMesh& QuadMesh::operator=(QuadMesh&& other) noexcept {
    if (this != &other) {
        release();
        m_vkCtx = other.m_vkCtx;
        m_vertexBuffer = other.m_vertexBuffer;
        m_allocation = other.m_allocation;
        other.m_vkCtx = nullptr;
        other.m_vertexBuffer = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
    }
    return *this;
}

void QuadMesh::release() {
    if (m_vkCtx && m_vertexBuffer != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_vkCtx->getAllocator(), m_vertexBuffer, m_allocation);
        m_vertexBuffer = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }
    m_vkCtx = nullptr;
}

void QuadMesh::draw(VkCommandBuffer cmd) const {
    if (cmd == VK_NULL_HANDLE || m_vertexBuffer == VK_NULL_HANDLE) return;
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer, &offset);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}