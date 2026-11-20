#ifndef GIRIGARA_WINDOW_WINDOW_MANAGER_H
#define GIRIGARA_WINDOW_WINDOW_MANAGER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <string>
#include <vector>
#include <array>
#include <cstdint>

#include <Event/EventBus.h>

class LoggerSystem;
class VulkanContext;

struct FrameSyncData {
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
};

class WindowManager {
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    WindowManager(int width, int height, const std::string& title,
                  VulkanContext& vkCtx, EventBus& eventBus, LoggerSystem& logger,
                  GLFWmonitor* monitor = nullptr);
    ~WindowManager();

    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;

    GLFWwindow* getWindow() const { return m_window; }
    bool shouldClose() const { return glfwWindowShouldClose(m_window); }
    void pollEvents() const { glfwPollEvents(); }
    bool isWindowFocused() const { return m_windowFocused; }

    
    // 步驟 1：取得 Swapchain 影像並開啟主 CommandBuffer 錄製（回傳 VK_NULL_HANDLE 代表最小化或重建中，略過本幀）
    VkCommandBuffer beginCommandBuffer(uint32_t& outImageIndex);
    // 步驟 2：轉換 Swapchain Image Layout 並開啟主視窗 Dynamic Rendering（在所有離屏 RenderTarget 畫完後呼叫）
    void beginSwapchainRendering(VkCommandBuffer cmd, uint32_t imageIndex);
    // 步驟 3：結束主視窗 Dynamic Rendering、轉為 PRESENT_SRC_KHR 並提交佇列
    void endFrame(uint32_t imageIndex);

    // 供 UIRenderer 與 RenderPass 查詢當前 Swapchain 狀態
    VkFormat getSwapchainImageFormat() const { return m_swapchainImageFormat; }
    VkExtent2D getSwapchainExtent() const { return m_swapchainExtent; }
    VkImage getSwapchainImage(uint32_t index) const { return m_swapchainImages[index]; }
    VkImageView getSwapchainImageView(uint32_t index) const { return m_swapchainImageViews[index]; }
    uint32_t getCurrentFrameIndex() const { return m_currentFrame; }

private:
    void createSwapchain();
    void cleanupSwapchain();
    void recreateSwapchain();
    void createSyncObjects();
    void cleanupSyncObjects();
    void updateScaleCache();

    // 事件處理（透過 EventBus 解耦）
    void onCursorPos(double xpos, double ypos);
    void onMouseButton(int button, int action, int mods);
    void onKey(int key, int scancode, int action, int mods);
    void onChar(unsigned int codepoint);
    void onWindowFocus(int focused);
    void onWindowSize(int width, int height);
    void onFramebufferSize(int width, int height);
    void onWindowContentScale(float xscale, float yscale);
    void onScroll(double xoffset, double yoffset);
    void onDrop(int count, const char** paths);

    // GLFW 靜態回調轉發
    static void staticCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void staticMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void staticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void staticCharCallback(GLFWwindow* window, unsigned int codepoint);
    static void staticWindowFocusCallback(GLFWwindow* window, int focused);
    static void staticWindowSizeCallback(GLFWwindow* window, int width, int height);
    static void staticFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void staticWindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale);
    static void staticScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void staticDropCallback(GLFWwindow* window, int count, const char** paths);

    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    double m_scaleX = 1.0;
    double m_scaleY = 1.0;
    bool m_windowFocused = true;
    bool m_framebufferResized = false;

    VulkanContext& m_vkCtx;
    EventBus& m_eventBus;
    LoggerSystem& m_logger;

    // 視窗專屬 Vulkan 表面與交換鏈
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    vkb::Swapchain m_vkbSwapchain;
    VkFormat m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_swapchainExtent{ 0, 0 };
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::array<FrameSyncData, MAX_FRAMES_IN_FLIGHT> m_frames{};
    uint32_t m_currentFrame = 0;
};

#endif // GIRIGARA_WINDOW_WINDOW_MANAGER_H