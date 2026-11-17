#include <Tool/ToolRuntime.h>
#include <Scene/LuaScene.h>
#include <Window/WindowManager.h>
#include <Resource/ScriptResource.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <filesystem>

namespace fs = std::filesystem;

ToolRuntime::ToolRuntime(int width, int height, const std::string& title)
: m_context(std::make_unique<ToolContext>(width, height, title)) {

    m_context->logger->info("[ToolRuntime] 工具運行時期啟動中...");

    initSubsystems();

    m_context->logger->info("[ToolRuntime] 工具運行時期就緒。");
}

ToolRuntime::~ToolRuntime() {
    // 顯式清空上層管理器，確保場景與 UI 的釋放早於 ToolContext 銷毀
    m_uiManager.reset();
    m_sceneManager.reset();
}

void ToolRuntime::initSubsystems() {
    // 1. 取得視窗尺寸與 Viewport 設定
    int fbWidth = 960, fbHeight = 640;
    glfwGetFramebufferSize(m_context->window->getWindow(), &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // 2. 載入預設字型 ("default") — 必須在 UIManager 建構之前完成
    m_context->toolResources->loadFont("default", "./assets/font/NaikaiFont-Regular.ttf", 48);

    // 3. 載入 UI 與 Console 背景 Shader，並配置給視窗投影
    m_uiShader = &m_context->toolResources->loadShader("ui", "./assets/shaders/ui.vert", "./assets/shaders/ui.frag");

    m_context->window->setShader(m_uiShader);
    m_context->window->updateProjection(fbWidth, fbHeight);

    // 載入系統基礎 Lua 腳本（工具亦可共用 UI 設定註冊等腳本）
    loadSystemScripts("./assets/sysScripts");

    // 4. 建立場景管理器
    m_sceneManager = std::make_unique<SceneManager>(*m_context);

    // 5. 建立 UIManager (會自動在 bindEvents() 訂閱 SceneUIChangedEvent)
    m_uiManager = std::make_unique<UIManager>(
        *m_uiShader,
        *m_context->eventBus,
        *m_context->keyboard,
        *m_context->console,
        *m_context->logger,
        *m_context->toolResources,
        static_cast<float>(fbWidth),
                                              static_cast<float>(fbHeight)
    );
}

void ToolRuntime::loadSystemScripts(const std::string& directoryPath) {
    fs::path dir(directoryPath);

    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        m_context->logger->warn("[Script] 找不到系統腳本資料夾: " + directoryPath);
        return;
    }

    m_context->logger->info("[Script] 開始載入系統腳本目錄: " + directoryPath);

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            std::string filePath = entry.path().string();
            std::string scriptName = entry.path().stem().string();

            try {
                m_context->toolResources->loadScript(scriptName, filePath);
                const auto& scriptRes = m_context->toolResources->get<ScriptResource>(scriptName);

                if (m_context->scriptEngine->executeResource(scriptRes)) {
                    m_context->logger->info("[Script] 已註冊系統腳本: " + scriptName + " (" + filePath + ")");
                } else {
                    m_context->logger->error("[Script] 執行腳本出錯: " + filePath);
                }
            } catch (const std::exception& e) {
                m_context->logger->error(std::string("[Script] 載入例外: ") + e.what());
            }
        }
    }
}

void ToolRuntime::run() {
    m_isRunning = true;
    float lastTime = static_cast<float>(glfwGetTime());

    while (m_isRunning && !m_context->window->shouldClose()) {
        float currentTime = static_cast<float>(glfwGetTime());
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        // 1. 輪詢視窗與輸入事件
        m_context->window->pollEvents();
		
		// 1.5 派發背景執行緒排入的事件（如 GpkPackCompletedEvent，並在主執行緒觸發 Lua 回呼）
        m_context->eventBus->dispatchQueued();

        // 2. 邏輯 Tick (純場景與 UI，無網路輪詢)
        m_sceneManager->update(dt);
        m_uiManager->update(dt);

        // 3. 清空緩衝區
        glClearColor(0.10f, 0.11f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 4. 渲染流程
        if (auto* scene = m_sceneManager->getCurrentScene()) {
            scene->renderWorld();
        }
        m_uiManager->render();

        // 5. 交換前後緩衝區
        m_context->window->swapBuffers();
    }
}
