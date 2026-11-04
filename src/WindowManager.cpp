#include <Window/WindowManager.h>
#include <Event/Event.h>
#include <stdexcept>
#include <iostream>
#include <System/LoggerSystem.h>

WindowManager::WindowManager(int width, int height, const std::string& title, EventBus& eventBus, LoggerSystem& logger)
: m_width(width), m_height(height), m_eventBus(eventBus), m_logger(logger) {

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        throw std::runtime_error("[WindowManager] Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);
    m_logger.info("[WindowManager] 視窗建立成功");

    // 將 this 指針綁定至視窗使用者資料
    glfwSetWindowUserPointer(m_window, this);

    // 註冊 GLFW 回調函式
    glfwSetCursorPosCallback(m_window, staticCursorPosCallback);
    glfwSetMouseButtonCallback(m_window, staticMouseButtonCallback);
    glfwSetKeyCallback(m_window, staticKeyCallback);
    glfwSetCharCallback(m_window, staticCharCallback);
    glfwSetWindowFocusCallback(m_window, staticWindowFocusCallback);
    glfwSetFramebufferSizeCallback(m_window, staticFramebufferSizeCallback);
    glfwSetScrollCallback(m_window, staticScrollCallback);
    m_logger.info("[WindowManager] GLFW callback function註冊完畢");
}

WindowManager::~WindowManager() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
}

void WindowManager::updateProjection(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_shader && width > 0 && height > 0) {
        m_shader->use();

        // 原生 OpenGL 欄優先 (Column-major) 4x4 正交矩陣 (左上為 0,0，右下為 width,height)
        float ortho[16] = {
            2.0f / static_cast<float>(width),  0.0f,                              0.0f,  0.0f,
            0.0f,                             -2.0f / static_cast<float>(height), 0.0f,  0.0f,
            0.0f,                              0.0f,                             -1.0f,  0.0f,
            -1.0f,                              1.0f,                              0.0f,  1.0f
        };

        m_shader->setMat4("projection", ortho);
    }
}

// -------------------------------------------------------------
// 成員事件處理 (統一透過 EventBus 發布事件)
// -------------------------------------------------------------

void WindowManager::onCursorPos(double xpos, double ypos) {
    int winWidth, winHeight;
    int fbWidth, fbHeight;
    glfwGetWindowSize(m_window, &winWidth, &winHeight);
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

    // 計算視窗邏輯像素與 Framebuffer 像素之間的縮放比例
    double scaleX = (winWidth > 0) ? (static_cast<double>(fbWidth) / winWidth) : 1.0;
    double scaleY = (winHeight > 0) ? (static_cast<double>(fbHeight) / winHeight) : 1.0;

    double actualX = xpos * scaleX;
    double actualY = ypos * scaleY;

    m_eventBus.publish(MouseMovedEvent(actualX, actualY));
}

void WindowManager::onMouseButton(int button, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);

    int winWidth, winHeight;
    int fbWidth, fbHeight;
    glfwGetWindowSize(m_window, &winWidth, &winHeight);
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

    double scaleX = (winWidth > 0) ? (static_cast<double>(fbWidth) / winWidth) : 1.0;
    double scaleY = (winHeight > 0) ? (static_cast<double>(fbHeight) / winHeight) : 1.0;

    double actualX = xpos * scaleX;
    double actualY = ypos * scaleY;

    m_eventBus.publish(MouseButtonEvent(button, action, mods, actualX, actualY));
}

void WindowManager::onKey(int key, int scancode, int action, int mods) {
    // 1. 發布通用鍵盤事件 (供 UIConsole 優先攔截 Enter、Backspace 等)
    KeyEvent keyEvent(key, scancode, action, mods);
    m_eventBus.publish(keyEvent);

    // 2. 若未被 Console 攔截，且為按下/放開，可依需求向下分發具體語意事件
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

void WindowManager::onFramebufferSize(int width, int height) {
    glViewport(0, 0, width, height);
    updateProjection(width, height);

    // ★ 發布視窗大小改變事件
    m_eventBus.publish(WindowResizeEvent(width, height));
}

void WindowManager::onScroll(double xoffset, double yoffset) {
    // 檢查 GLFW 視窗當前是否按著 Ctrl / Shift / Alt
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

// -------------------------------------------------------------
// GLFW 靜態回調函式轉發
// -------------------------------------------------------------

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
    if (self) {
        self->onScroll(xoffset, yoffset);
    }
}
