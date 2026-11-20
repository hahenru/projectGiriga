#include <Window/WindowManager.h>
#include <Renderer/VulkanContext.h>
#include <Event/Event.h>
#include <System/LoggerSystem.h>
#include <stdexcept>

WindowManager::WindowManager(int width, int height, const std::string& title,
                             VulkanContext& vkCtx, EventBus& eventBus, LoggerSystem& logger,
                             GLFWmonitor* monitor)
    : m_width(width), m_height(height), m_vkCtx(vkCtx), m_eventBus(eventBus), m_logger(logger) {

    m_window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
    if (!m_window) {
        throw std::runtime_error("[WindowManager] Failed to create GLFW window");
    }

    if (glfwCreateWindowSurface(m_vkCtx.getInstance(), m_window, nullptr, &m_surface) != VK_SUCCESS) {
        glfwDestroyWindow(m_window);
        throw std::runtime_error("[WindowManager] Failed to create Vulkan window surface");
    }

    createSwapchain();
    createSyncObjects();

    m_logger.info("[WindowManager] Vulkan 視窗與 Swapchain 建立成功: " + title);

    updateScaleCache();
    glfwSetWindowUserPointer(m_window, this);

    glfwSetCursorPosCallback(m_window, staticCursorPosCallback);
    glfwSetMouseButtonCallback(m_window, staticMouseButtonCallback);
    glfwSetKeyCallback(m_window, staticKeyCallback);
    glfwSetCharCallback(m_window, staticCharCallback);
    glfwSetWindowFocusCallback(m_window, staticWindowFocusCallback);
    glfwSetWindowSizeCallback(m_window, staticWindowSizeCallback);
    glfwSetFramebufferSizeCallback(m_window, staticFramebufferSizeCallback);
    glfwSetWindowContentScaleCallback(m_window, staticWindowContentScaleCallback);
    glfwSetScrollCallback(m_window, staticScrollCallback);
    glfwSetDropCallback(m_window, staticDropCallback);
}

WindowManager::~WindowManager() {
    m_vkCtx.waitIdle();

    cleanupSyncObjects();
    cleanupSwapchain();

    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_vkCtx.getInstance(), m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    if (m_window) {
        glfwSetWindowUserPointer(m_window, nullptr);
        glfwSetCursorPosCallback(m_window, nullptr);
        glfwSetMouseButtonCallback(m_window, nullptr);
        glfwSetKeyCallback(m_window, nullptr);
        glfwSetCharCallback(m_window, nullptr);
        glfwSetWindowFocusCallback(m_window, nullptr);
        glfwSetWindowSizeCallback(m_window, nullptr);
        glfwSetFramebufferSizeCallback(m_window, nullptr);
        glfwSetWindowContentScaleCallback(m_window, nullptr);
        glfwSetScrollCallback(m_window, nullptr);
        glfwSetDropCallback(m_window, nullptr);

        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
}

void WindowManager::createSwapchain() {
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    if (fbWidth == 0 || fbHeight == 0) return;

    vkb::SwapchainBuilder builder{
        m_vkCtx.getPhysicalDevice(),
        m_vkCtx.getDevice(),
        m_surface
    };

    auto swapRet = builder
        .set_desired_format({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(static_cast<uint32_t>(fbWidth), static_cast<uint32_t>(fbHeight))
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build();

    if (!swapRet) {
        throw std::runtime_error("[WindowManager] Failed to build Swapchain: " + swapRet.error().message());
    }

    m_vkbSwapchain = swapRet.value();
    m_swapchainImageFormat = m_vkbSwapchain.image_format;
    m_swapchainExtent = m_vkbSwapchain.extent;
    m_swapchainImages = m_vkbSwapchain.get_images().value();
    m_swapchainImageViews = m_vkbSwapchain.get_image_views().value();
}

void WindowManager::cleanupSwapchain() {
    if (!m_swapchainImageViews.empty()) {
        m_vkbSwapchain.destroy_image_views(m_swapchainImageViews);
        m_swapchainImageViews.clear();
    }
    m_swapchainImages.clear();
    if (m_vkbSwapchain.swapchain != VK_NULL_HANDLE) {
        vkb::destroy_swapchain(m_vkbSwapchain);
        m_vkbSwapchain.swapchain = VK_NULL_HANDLE;
    }
}

void WindowManager::recreateSwapchain() {
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    if (fbWidth == 0 || fbHeight == 0) return;

    m_vkCtx.waitIdle();
    cleanupSwapchain();
    createSwapchain();
}

void WindowManager::createSyncObjects() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_vkCtx.getGraphicsQueueFamily();
    vkCreateCommandPool(m_vkCtx.getDevice(), &poolInfo, nullptr, &m_commandPool);

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = m_commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> cmds{};
    vkAllocateCommandBuffers(m_vkCtx.getDevice(), &cmdAllocInfo, cmds.data());

    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_frames[i].commandBuffer = cmds[i];
        vkCreateSemaphore(m_vkCtx.getDevice(), &semInfo, nullptr, &m_frames[i].imageAvailableSemaphore);
        vkCreateSemaphore(m_vkCtx.getDevice(), &semInfo, nullptr, &m_frames[i].renderFinishedSemaphore);
        vkCreateFence(m_vkCtx.getDevice(), &fenceInfo, nullptr, &m_frames[i].inFlightFence);
    }
}

void WindowManager::cleanupSyncObjects() {
    VkDevice dev = m_vkCtx.getDevice();
    for (auto& frame : m_frames) {
        if (frame.inFlightFence != VK_NULL_HANDLE) vkDestroyFence(dev, frame.inFlightFence, nullptr);
        if (frame.renderFinishedSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(dev, frame.renderFinishedSemaphore, nullptr);
        if (frame.imageAvailableSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(dev, frame.imageAvailableSemaphore, nullptr);
        frame = {};
    }
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(dev, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
}

VkCommandBuffer WindowManager::beginCommandBuffer(uint32_t& outImageIndex) {
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    if (fbWidth == 0 || fbHeight == 0) return VK_NULL_HANDLE;

    if (m_framebufferResized || m_vkbSwapchain.swapchain == VK_NULL_HANDLE) {
        m_framebufferResized = false;
        recreateSwapchain();
        if (m_vkbSwapchain.swapchain == VK_NULL_HANDLE) return VK_NULL_HANDLE;
    }

    VkDevice dev = m_vkCtx.getDevice();
    FrameSyncData& frame = m_frames[m_currentFrame];

    vkWaitForFences(dev, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);

    VkResult res = vkAcquireNextImageKHR(
        dev, m_vkbSwapchain.swapchain, UINT64_MAX,
        frame.imageAvailableSemaphore, VK_NULL_HANDLE, &outImageIndex
    );

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return VK_NULL_HANDLE;
    } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("[WindowManager] Failed to acquire swapchain image");
    }

    vkResetFences(dev, 1, &frame.inFlightFence);
    vkResetCommandBuffer(frame.commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(frame.commandBuffer, &beginInfo);

    return frame.commandBuffer;
}

void WindowManager::beginSwapchainRendering(VkCommandBuffer cmd, uint32_t imageIndex) {
    // 轉換 Swapchain Image Layout: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL (Vulkan 1.3 Sync2)
    VkImageMemoryBarrier2 imgBarrier{};
    imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imgBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    imgBarrier.srcAccessMask = 0;
    imgBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    imgBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imgBarrier.image = m_swapchainImages[imageIndex];
    imgBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imgBarrier;
    vkCmdPipelineBarrier2(cmd, &depInfo);

    // 開啟主視窗 Dynamic Rendering 並執行深灰背景清屏
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = m_swapchainImageViews[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{ 0.12f, 0.12f, 0.14f, 1.0f }};

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.renderArea = { {0, 0}, m_swapchainExtent };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(cmd, &renderInfo);
}

void WindowManager::endFrame(uint32_t imageIndex) {
    FrameSyncData& frame = m_frames[m_currentFrame];

    vkCmdEndRendering(frame.commandBuffer);

    // 轉換 Swapchain Image Layout: COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
    VkImageMemoryBarrier2 presentBarrier{};
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    presentBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    presentBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    presentBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    presentBarrier.dstAccessMask = 0;
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.image = m_swapchainImages[imageIndex];
    presentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &presentBarrier;
    vkCmdPipelineBarrier2(frame.commandBuffer, &depInfo);

    vkEndCommandBuffer(frame.commandBuffer);

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &frame.imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &frame.renderFinishedSemaphore;

    vkQueueSubmit(m_vkCtx.getGraphicsQueue(), 1, &submitInfo, frame.inFlightFence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &frame.renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_vkbSwapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VkResult res = vkQueuePresentKHR(m_vkCtx.getPresentQueue(), &presentInfo);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        recreateSwapchain();
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void WindowManager::updateScaleCache() {
    int winWidth = 0, winHeight = 0;
    int fbWidth = 0, fbHeight = 0;
    glfwGetWindowSize(m_window, &winWidth, &winHeight);
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

    if (winWidth > 0 && winHeight > 0 && fbWidth > 0 && fbHeight > 0) {
        m_scaleX = static_cast<double>(fbWidth) / static_cast<double>(winWidth);
        m_scaleY = static_cast<double>(fbHeight) / static_cast<double>(winHeight);
    }
}

void WindowManager::onCursorPos(double xpos, double ypos) {
    m_eventBus.publish(MouseMovedEvent(xpos * m_scaleX, ypos * m_scaleY));
}

void WindowManager::onMouseButton(int button, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);
    m_eventBus.publish(MouseButtonEvent(button, action, mods, xpos * m_scaleX, ypos * m_scaleY));
}

void WindowManager::onKey(int key, int scancode, int action, int mods) {
    KeyEvent keyEvent(key, scancode, action, mods);
    m_eventBus.publish(keyEvent);

    if (!keyEvent.isHandled) {
        if (action == GLFW_PRESS) {
            m_eventBus.publish(KeyPressedEvent(key, scancode, mods));
        } else if (action == GLFW_RELEASE) {
            m_eventBus.publish(KeyReleasedEvent(key, scancode, mods));
        }
    }
}

void WindowManager::onChar(unsigned int codepoint) {
    m_eventBus.publish(CharInputEvent(codepoint));
}

void WindowManager::onWindowFocus(int focused) {
    m_windowFocused = (focused == GLFW_TRUE);
    m_eventBus.publish(WindowFocusEvent(m_windowFocused));
}

void WindowManager::onWindowSize(int /*width*/, int /*height*/) {
    updateScaleCache();
}

void WindowManager::onFramebufferSize(int width, int height) {
    updateScaleCache();
    m_framebufferResized = true;
    m_eventBus.publish(WindowResizeEvent(width, height));
}

void WindowManager::onWindowContentScale(float /*xscale*/, float /*yscale*/) {
    updateScaleCache();
}

void WindowManager::onScroll(double xoffset, double yoffset) {
    int mods = 0;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        mods |= GLFW_MOD_CONTROL;
    }
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(m_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        mods |= GLFW_MOD_SHIFT;
    }
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(m_window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
        mods |= GLFW_MOD_ALT;
    }

    MouseScrolledEvent event{xoffset, yoffset, mods};
    m_eventBus.publish(event);
}

void WindowManager::onDrop(int count, const char** paths) {
    if (count <= 0 || !paths) return;
    std::vector<std::string> dropped;
    dropped.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        if (paths[i]) dropped.emplace_back(paths[i]);
    }
    if (!dropped.empty()) {
        m_eventBus.publish(FileDropEvent(std::move(dropped)));
    }
}

void WindowManager::staticCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* self = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (self) self->onCursorPos(xpos, ypos);
}

void WindowManager::staticMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* self = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (self) self->onMouseButton(button, action, mods);
}

void WindowManager::staticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* self = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (self) self->onKey(key, scancode, action, mods);
}

void WindowManager::staticCharCallback(GLFWwindow* window, unsigned int codepoint) {
    auto* self = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (self) self->onChar(codepoint);
}

void WindowManager::staticWindowFocusCallback(GLFWwindow* window, int focused) {
    auto* self = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (self) self->onWindowFocus(focused);
}

void WindowManager::staticFramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (self) self->onFramebufferSize(width, height);
}

void WindowManager::staticScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* self = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (self) self->onScroll(xoffset, yoffset);
}

void WindowManager::staticWindowSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (self) self->onWindowSize(width, height);
}

void WindowManager::staticWindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale) {
    auto* self = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (self) self->onWindowContentScale(xscale, yscale);
}

void WindowManager::staticDropCallback(GLFWwindow* window, int count, const char** paths) {
    auto* self = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (self) self->onDrop(count, paths);
}