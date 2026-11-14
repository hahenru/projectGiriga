#include <Client/ClientContext.h>

ClientContext::ClientContext(int width, int height, const std::string& title)
    : CoreContext(),
      glfw(std::make_unique<GlfwContext>()),
      window(std::make_unique<WindowManager>(width, height, title, *eventBus, *logger)) {

    logger->info("[ClientContext] GLFW 視窗建立完成。");

    // 載入 GLAD 與 OpenGL 驅動函式
    GlfwContext::initGLAD();
    logger->info("[ClientContext] OpenGL 驅動函式載入 (GLAD) 完成。");

    // 初始化客戶端音訊、輸入、網路與實體資源管理器
    audio = std::make_unique<AudioSystem>(*eventBus);
    keyboard = std::make_unique<KeyboardSystem>(*eventBus);
    enetCtx = std::make_unique<ENetContext>();
    network = std::make_unique<ENetNode>(*logger);
    clientResources = std::make_unique<ClientResourceManager>(*logger);

    // ★ 關鍵：將衍生類別的實體指針掛給基底的抽象指針
    this->resources = clientResources.get();

    logger->info("[ClientContext] 客戶端硬體與網路子系統就緒。");
}

ClientContext::~ClientContext() {
    if (logger) {
        logger->info("[ClientContext] 釋放客戶端硬體與網路子系統...");
    }

    // 嚴格逆序釋放：先釋放 network 再釋放 enetCtx，接著才是硬體資源
    network.reset();
    enetCtx.reset();
    clientResources.reset();
    keyboard.reset();
    audio.reset();
    window.reset();
    glfw.reset();
}
