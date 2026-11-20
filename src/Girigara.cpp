#include <Girigara.h>
#include <Resource/ScriptResource.h>
#include <Server/ServerConfig.h>
#include <Scene/EngineScene.h>
#include <chrono>
#include <filesystem>

#ifndef GIRIGARA_SERVER
#include <GLFW/glfw3.h>
#endif

namespace fs = std::filesystem;

Girigara::Girigara(const GirigaraConfig& config)
    : m_config(config),
      m_context(std::make_unique<CoreContext>()) {

#ifdef GIRIGARA_SERVER
    m_config.headless = true;
#endif

    if (m_config.headless) {
        m_context->logger->info("[Girigara] 引擎啟動中（無頭模式 / Headless）...");
        initHeadlessSubsystems();
    } else {
#ifndef GIRIGARA_SERVER
        m_context->logger->info("[Girigara] 引擎啟動中（圖形視窗模式）...");
        initGraphicalSubsystems();
#endif
    }

    m_context->logger->info("[Girigara] 引擎核心就緒。");
}

Girigara::~Girigara() {
#ifndef GIRIGARA_SERVER
    stopIntegratedServer();
#endif

    if (m_context && m_context->logger) {
        m_context->logger->info("[Girigara] 正在關閉引擎與釋放子系統...");
    }

    // 嚴格逆序釋放：先 UI 與場景，再網路與資源，最後硬體視窗與 CoreContext
#ifndef GIRIGARA_SERVER
    m_uiManager.reset();
#endif
    m_sceneManager.reset();

    m_network.reset();
    m_enetCtx.reset();

#ifndef GIRIGARA_SERVER
    m_clientResources.reset();
    m_audio.reset();
    m_keyboard.reset();
    m_windows.clear();
    m_glfw.reset();
#endif

    m_serverResources.reset();
    m_context.reset();
}

void Girigara::initHeadlessSubsystems() {
    m_serverResources = std::make_unique<ServerResourceManager>(*m_context->logger);
    m_context->resources = m_serverResources.get();

    if (m_config.enableNetwork) {
        m_enetCtx = std::make_unique<ENetContext>();
        m_network = std::make_unique<ENetNode>(*m_context->logger);
    }

    loadSystemScripts("./assets/sysScripts");
    m_sceneManager = std::make_unique<SceneManager>(*m_context);
}

#ifndef GIRIGARA_SERVER
void Girigara::initGraphicalSubsystems() {
    // 1. 初始化 GLFW、全局 Vulkan 裝置與主視窗（預留 m_windows 支援未來多視窗）
    m_glfw = std::make_unique<GlfwContext>();
    m_vkCtx = std::make_unique<VulkanContext>(*m_context->logger, true);

    auto mainWin = std::make_unique<WindowManager>(
        m_config.width, m_config.height, m_config.title,
        *m_vkCtx, *m_context->eventBus, *m_context->logger
    );

    int fbWidth = m_config.width;
    int fbHeight = m_config.height;
    glfwGetFramebufferSize(mainWin->getWindow(), &fbWidth, &fbHeight);
    m_windows.push_back(std::move(mainWin));

    // 2. 初始化輸入、音訊、網路與資源管理器
    m_keyboard = std::make_unique<KeyboardSystem>(*m_context->eventBus);
    if (m_config.enableAudio) {
        m_audio = std::make_unique<AudioSystem>(*m_context->eventBus);
    }
    if (m_config.enableNetwork) {
        m_enetCtx = std::make_unique<ENetContext>();
        m_network = std::make_unique<ENetNode>(*m_context->logger);
    }

    m_clientResources = std::make_unique<ClientResourceManager>(*m_context->logger);
    m_context->resources = m_clientResources.get();

    // 3. 載入預設字型（註：Shader 將於下一步改為載入 SPIR-V 與 VulkanPipeline）
    m_clientResources->loadFont("default", "./assets/font/NaikaiFont-Regular.ttf", 48);

    // 4. 載入系統腳本並建立場景管理器
    loadSystemScripts("./assets/sysScripts");
    m_sceneManager = std::make_unique<SceneManager>(*m_context);

    // 5. 建立純淨 UIManager，並依設定掛載內建開發者工具
    const float w = static_cast<float>(fbWidth);
    const float h = static_cast<float>(fbHeight);

    m_uiManager = std::make_unique<UIManager>(
        *m_vkCtx,
        m_windows.front()->getSwapchainImageFormat(),
        *m_context->eventBus,
        *m_clientResources,
        w, h
    );

    if (m_config.enableDevTools) {
        initDevTools(w, h);
    }
}
#endif

#ifndef GIRIGARA_SERVER
void Girigara::initDevTools(float width, float height) {
    // 掛載系統/開發者設定視窗
    m_devSettingsMenu = m_uiManager->emplaceOverlay<UISettingsMenu>(
        0.0f, 0.0f, width, height,
        *m_clientResources, *m_context->eventBus
    );
    m_devSettingsMenu->setVisible(false);

    // 掛載內建開發者 Console（確保 Z-order 在設定視窗之上）
    m_devConsole = m_uiManager->emplaceOverlay<UIConsole>(
        0.0f, 0.0f, width, height,
        *m_context->eventBus, *m_context->console, *m_context->logger, *m_clientResources
    );
    m_devConsole->setName("InGameConsole");
    m_devConsole->setVisible(false);

    // ~ 鍵：切換開發者 Console
    m_keyboard->registerShortcut(GLFW_KEY_GRAVE_ACCENT, 0, [this]() {
        if (!m_devConsole) return;
        bool next = !m_devConsole->isVisible();
        m_devConsole->setVisible(next);
        if (next) {
            m_uiManager->bringOverlayToFront(m_devConsole);
            if (auto* input = m_devConsole->getInputField()) {
                input->setFocused(true);
            }
        } else {
            m_uiManager->getRootUI().setFocusedElement(nullptr);
            if (auto* input = m_devConsole->getInputField()) {
                input->setFocused(false);
            }
        }
    });

    // ESC 鍵：若 Console 開著優先關 Console，否則切換系統/開發者設定視窗
    m_keyboard->registerShortcut(GLFW_KEY_ESCAPE, 0, [this]() {
        if (m_devConsole && m_devConsole->isVisible()) {
            m_devConsole->setVisible(false);
            m_uiManager->getRootUI().setFocusedElement(nullptr);
            if (auto* input = m_devConsole->getInputField()) {
                input->setFocused(false);
            }
            return;
        }
        if (!m_devSettingsMenu) return;
        bool next = !m_devSettingsMenu->isVisible();
        m_devSettingsMenu->setVisible(next);
        if (next) {
            m_uiManager->bringOverlayToFront(m_devSettingsMenu);
        } else if (m_uiManager->getRootUI().getFocusedElement() == m_devSettingsMenu) {
            m_uiManager->getRootUI().setFocusedElement(nullptr);
        }
    });
}
#endif

void Girigara::loadSystemScripts(const std::string& directoryPath) {
    fs::path dir(directoryPath);
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        m_context->logger->warn("[Script] 找不到系統腳本資料夾: " + directoryPath);
        return;
    }

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            std::string filePath = entry.path().string();
            std::string scriptName = entry.path().stem().string();
            try {
                if (m_config.headless) {
                    m_serverResources->loadScript(scriptName, filePath);
                } else {
#ifndef GIRIGARA_SERVER
                    m_clientResources->loadScript(scriptName, filePath);
#endif
                }
                const auto& scriptRes = m_context->resources->get<ScriptResource>(scriptName);
                if (m_context->scriptEngine->executeResource(scriptRes)) {
                    m_context->logger->info("[Script] 已載入系統腳本: " + scriptName);
                } else {
                    m_context->logger->error("[Script] 執行腳本失敗: " + filePath);
                }
            } catch (const std::exception& e) {
                m_context->logger->error(std::string("[Script] 載入例外: ") + e.what());
            }
        }
    }
}

bool Girigara::startServer(uint16_t port, size_t maxClients) {
    if (!m_network) return false;
    return m_network->startServer(port, maxClients);
}

bool Girigara::connectToServer(const std::string& host, uint16_t port) {
    if (!m_network) return false;
    return m_network->connectClient(host, port);
}

#ifndef GIRIGARA_SERVER
void Girigara::startIntegratedServer(uint16_t port, int tickRate) {
    if (m_integratedServer) return;

    GirigaraConfig srvCfg;
    srvCfg.headless = true;
    srvCfg.tickRate = tickRate;
    srvCfg.enableNetwork = true;

    m_integratedServer = std::make_unique<Girigara>(srvCfg);
    m_integratedServer->startServer(port, 32);

    m_integratedServerThread = std::jthread([this]() {
        m_integratedServer->run();
    });
}

void Girigara::stopIntegratedServer() {
    if (m_integratedServer) {
        m_integratedServer->stop();
        if (m_integratedServerThread.joinable()) {
            m_integratedServerThread.join();
        }
        m_integratedServer.reset();
    }
}
#endif

void Girigara::run() {
    if (m_config.headless) {
        runHeadlessLoop();
    } else {
#ifndef GIRIGARA_SERVER
        runGraphicalLoop();
#endif
    }
}

void Girigara::runHeadlessLoop() {
    m_isRunning = true;
    const int rate = (m_config.tickRate > 0) ? m_config.tickRate : 20;
    const auto tickDuration = std::chrono::duration<double>(1.0 / static_cast<double>(rate));
    auto lastTime = std::chrono::steady_clock::now();

    while (m_isRunning) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        if (m_network) {
            m_network->poll(0);
        }
        m_context->eventBus->dispatchQueued();
        m_sceneManager->update(dt);

        auto elapsed = std::chrono::steady_clock::now() - now;
        if (elapsed < tickDuration) {
            std::this_thread::sleep_for(tickDuration - elapsed);
        }
    }
}

#ifndef GIRIGARA_SERVER
void Girigara::runGraphicalLoop() {
    if (m_windows.empty()) return;

    m_isRunning = true;
    float lastTime = static_cast<float>(glfwGetTime());

    while (m_isRunning && !m_windows.front()->shouldClose()) {
        float currentTime = static_cast<float>(glfwGetTime());
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        // 1. 輪詢所有視窗事件與非同步事件佇列
        for (auto& win : m_windows) {
            win->pollEvents();
        }
        m_context->eventBus->dispatchQueued();

        // 2. 網路輪詢
        if (m_network) {
            m_network->poll(0);
        }

        // 3. 邏輯更新（場景與 UI）
        m_sceneManager->update(dt);
        m_uiManager->update(dt);

        // 4. 畫面渲染（多視窗 Vulkan 雙階段 Dynamic Rendering 管線）
		for (size_t i = 0; i < m_windows.size(); ++i) {
			auto& win = m_windows[i];
			uint32_t imageIndex = 0;
			VkCommandBuffer cmd = win->beginCommandBuffer(imageIndex);
			if (cmd == VK_NULL_HANDLE) continue;

			// Pass 1（主視窗專屬）：若當前為 EngineScene，先將所有內嵌 UIViewport 離屏渲染完畢
			if (i == 0) {
				if (auto* engineScene = dynamic_cast<EngineScene*>(m_sceneManager->getCurrentScene())) {
					engineScene->renderViewportsOffscreen(cmd);
				}
			}

			// Pass 2：開啟主視窗 Swapchain Dynamic Rendering，繪製場景 UI、UIWindow 視口與開發者 Console
			win->beginSwapchainRendering(cmd, imageIndex);
			if (i == 0 && m_uiManager) {
				m_uiManager->render(cmd);
			}
			win->endFrame(imageIndex);
		}
	}
}
#endif
