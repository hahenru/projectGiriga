#ifndef GIRIGARA_WINDOW_GLFW_CONTEXT_H
#define GIRIGARA_WINDOW_GLFW_CONTEXT_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>

class GlfwContext {
public:
    GlfwContext() {
        if (!glfwInit()) {
            throw std::runtime_error("[GlfwContext] Failed to initialize GLFW");
        }

        if (!glfwVulkanSupported()) {
            glfwTerminate();
            throw std::runtime_error("[GlfwContext] GLFW reports Vulkan is not supported");
        }

        // 禁止 GLFW 建立 OpenGL Context，改由 Vulkan 接管渲染表面
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    ~GlfwContext() {
        glfwTerminate();
    }

    GlfwContext(const GlfwContext&) = delete;
    GlfwContext& operator=(const GlfwContext&) = delete;
    GlfwContext(GlfwContext&&) = delete;
    GlfwContext& operator=(GlfwContext&&) = delete;
};

#endif // GIRIGARA_WINDOW_GLFW_CONTEXT_H
