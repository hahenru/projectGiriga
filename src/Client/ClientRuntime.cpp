#include <Client/ClientRuntime.h>
#include <Scene/LuaScene.h>
#include <Window/WindowManager.h>
#include <Resource/ScriptResource.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <filesystem>

namespace fs = std::filesystem;

ClientRuntime::ClientRuntime(int width, int height, const std::string& title)
    : m_context(std::make_unique<ClientContext>(width, height, title)) {

    m_context->logger->info("[ClientRuntime] 客戶端運行時期啟動中...");

    initSubsystems();

    m_context->logger->info("[ClientRuntime] 客戶端運行時期就緒。");
}

ClientRuntime::~ClientRuntime() {
    // 顯式清空上層管理器，確保場景與 UI 的釋放早於 ClientContext 銷毀
    m_uiManager.reset();
    m_sceneManager.reset();
}

void ClientRuntime::initSubsystems() {
    // 1. 取得視窗尺寸與 Viewport 設定
    int fbWidth = 800, fbHeight = 600;
    glfwGetFramebufferSize(m_context->window->getWindow(), &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // 2. 載入預設字型 ("default") — 必須在 UIManager 建構之前完成
    m_context->clientResources->loadFont("default", "./assets/font/NaikaiFont-Regular.ttf", 48);

    // 3. 載入 UI 與 Console 背景 Shader，並配置給視窗投影
    m_uiShader = &m_context->clientResources->loadShader("ui", "./assets/shaders/ui.vert", "./assets/shaders/ui.frag");
    m_context->clientResources->loadShader("ConsoleBackground", "./assets/shaders/console_bg.vert", "./assets/shaders/console_bg.frag");

    m_context->window->setShader(m_uiShader);
    m_context->window->updateProjection(fbWidth, fbHeight);

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
        *m_context->clientResources,
        static_cast<float>(fbWidth),
        static_cast<float>(fbHeight)
    );
}

void ClientRuntime::loadSystemScripts(const std::string& directoryPath) {
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
                m_context->clientResources->loadScript(scriptName, filePath);
                const auto& scriptRes = m_context->clientResources->get<ScriptResource>(scriptName);

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

void ClientRuntime::run() {
    m_isRunning = true;
    float lastTime = static_cast<float>(glfwGetTime());

    while (m_isRunning && !m_context->window->shouldClose()) {
        float currentTime = static_cast<float>(glfwGetTime());
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        // 1. 輪詢視窗與輸入事件
        m_context->window->pollEvents();

        // ★ 1.5 輪詢客戶端 ENet 網路封包（非阻塞 0ms）
        if (m_context->network) {
            m_context->network->poll(0);
        }
		
        // 2. 邏輯 Tick (場景與 UI 分開推進)
        m_sceneManager->update(dt);
        m_uiManager->update(dt);

        // 3. 視窗尺寸校正 (保持自適應縮放防跑版) 功能關閉，因為重複呼叫
        // int fbWidth = 0, fbHeight = 0;
        // glfwGetFramebufferSize(m_context->window->getWindow(), &fbWidth, &fbHeight);
		// m_uiManager->onWindowResize(static_cast<float>(fbWidth), static_cast<float>(fbHeight));

        // 4. 清空緩衝區
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 5. 渲染流程：純世界渲染 -> 上層 UI 渲染
        if (auto* scene = m_sceneManager->getCurrentScene()) {
            scene->renderWorld();
        }
        m_uiManager->render();

        // 6. 交換前後緩衝區
        m_context->window->swapBuffers();
    }
}