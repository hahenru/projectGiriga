#include <Girigara.h>
#include <Script/ScriptBindings.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace fs = std::filesystem;

Girigara::Girigara(int width, int height, const std::string& title)
: m_logger(65535),
  m_glfw(),
  m_eventBus(),
  m_keyboardSystem(m_eventBus),
  m_window(std::make_unique<WindowManager>(width, height, title, m_eventBus, m_logger)),
  m_resources(m_logger),
  m_settings(m_logger),
  m_scriptEngine(),
  m_console(m_logger) {

    m_logger.info("引擎初始化開始...");

    GlfwContext::initGLAD();

    initSubsystems();
    setupShortcuts();

    m_logger.info("引擎初始化完成。");
}

void Girigara::initSubsystems() {
    m_uiShader = &m_resources.loadShader("ui", "./assets/shaders/ui.vert", "./assets/shaders/ui.frag");
    m_window->setShader(m_uiShader);

    int fbWidth = 800, fbHeight = 600;
    glfwGetFramebufferSize(m_window->getWindow(), &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    m_window->updateProjection(fbWidth, fbHeight);

    // 1. 註冊 Lua C++ 綁定與連接 Console
    ScriptBindings::registerAll(m_scriptEngine.getRawState(), m_logger, m_audio, m_settings);
    m_console.setScriptEngine(&m_scriptEngine);

    // 2. 載入系統腳本
    loadSystemScripts("./assets/sysScripts");

    // 3. 建立場景管理器
    m_sceneManager = std::make_unique<SceneManager>(m_eventBus, m_resources, m_logger, m_audio);

    // 4. 建立 UIManage
    m_uiManager = std::make_unique<UIManager>(
        *m_uiShader,
        m_eventBus,
        m_keyboardSystem,
        m_console,
        m_logger,
        m_resources,
        static_cast<float>(fbWidth),
        static_cast<float>(fbHeight)
    );
}

void Girigara::loadSystemScripts(const std::string& directoryPath) {
    fs::path dir(directoryPath);

    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        m_logger.warn("[Script] 找不到系統腳本資料夾: " + directoryPath);
        return;
    }

    m_logger.info("[Script] 開始載入系統腳本目錄: " + directoryPath);

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            std::string filePath = entry.path().string();
            std::string scriptName = entry.path().stem().string();

            try {
                m_resources.loadScript(scriptName, filePath);
                const auto& scriptRes = m_resources.getScript(scriptName);

                if (m_scriptEngine.executeResource(scriptRes)) {
                    m_logger.info("[Script] 已註冊系統腳本: " + scriptName + " (" + filePath + ")");
                } else {
                    m_logger.error("[Script] 執行腳本出錯: " + filePath);
                }
            } catch (const std::exception& e) {
                m_logger.error(std::string("[Script] 載入例外: ") + e.what());
            }
        }
    }
}

void Girigara::setupShortcuts() {
    // 這裡只留未來真正的全域引擎級別操作 (例如 F11 切換全螢幕、F3 顯示 FPS 等)
    // UI 相關的波浪鍵與 ESC 快捷鍵已全權交由 UIManager 自行向 m_keyboardSystem 註冊！
}

void Girigara::run() {
    m_isRunning = true;
    float lastTime = static_cast<float>(glfwGetTime());

    while (m_isRunning && !m_window->shouldClose()) {
        float currentTime = static_cast<float>(glfwGetTime());
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        m_window->pollEvents();

        // 1. 邏輯更新（場景更新 + UIManager 三棵樹 Tick 更新）
        m_sceneManager->update(dt);
        m_uiManager->update(dt);

        // 2. 清空
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 3. 解析度更新廣播（供視窗縮放時自適應）
        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(m_window->getWindow(), &fbWidth, &fbHeight);
        m_uiManager->onWindowResize(static_cast<float>(fbWidth), static_cast<float>(fbHeight));

        // 4. 場景純世界渲染（不碰 UI）
        if (auto* scene = m_sceneManager->getCurrentScene()) {
            scene->renderWorld();
        }

        // 5. ★ 全權由 UIManager 渲染三棵樹（內部封裝 UIRenderer 與剪裁堆疊）
        m_uiManager->render();

        m_window->swapBuffers();
    }
}
