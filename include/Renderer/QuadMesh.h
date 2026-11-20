#ifndef GIRIGARA_RENDERER_QUAD_MESH_H
#define GIRIGARA_RENDERER_QUAD_MESH_H

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

class VulkanContext;

class QuadMesh {
public:
    explicit QuadMesh(VulkanContext& vkCtx);
    ~QuadMesh();

    QuadMesh(const QuadMesh&) = delete;
    QuadMesh& operator=(const QuadMesh&) = delete;

    QuadMesh(QuadMesh&& other) noexcept;
    QuadMesh& operator=(QuadMesh&& other) noexcept;

    void draw(VkCommandBuffer cmd) const;

private:
    void release();

    VulkanContext* m_vkCtx = nullptr;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
};

#endif // GIRIGARA_RENDERER_QUAD_MESH_H