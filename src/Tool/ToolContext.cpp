#include <Tool/ToolContext.h>

ToolContext::ToolContext(int width, int height, const std::string& title)
: CoreContext(),
glfw(std::make_unique<GlfwContext>()),
window(std::make_unique<WindowManager>(width, height, title, *eventBus, *logger)) {

    logger->info("[ToolContext] 工具視窗建立完成。");

    GlfwContext::initGLAD();
    logger->info("[ToolContext] OpenGL 驅動函式載入 (GLAD) 完成。");

    keyboard = std::make_unique<KeyboardSystem>(*eventBus);
    toolResources = std::make_unique<ToolResourceManager>(*logger);
    resources = toolResources.get();
    gpkBinder = std::make_unique<GpkScriptBinder>(*eventBus);

    logger->info("[ToolContext] 打包工具子系統就緒。");
}

ToolContext::~ToolContext() {
    if (logger) {
        logger->info("[ToolContext] 釋放打包工具子系統...");
    }

    // 嚴格逆序釋放
    gpkBinder.reset();
    resources = nullptr;
    toolResources.reset();
    keyboard.reset();
    window.reset();
    glfw.reset();
}
