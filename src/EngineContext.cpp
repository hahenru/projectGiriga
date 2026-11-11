#include <Core/EngineContext.h>
#include <System/LoggerSystem.h>
#include <System/ConsoleSystem.h>
#include <Window/GlfwContext.h>
#include <Window/WindowManager.h>
#include <Event/EventBus.h>
#include <System/KeyboardSystem.h>
#include <Resource/ResourceManager.h>
#include <Script/ScriptEngine.h>
#include <Audio/AudioSystem.h>
#include <System/SettingsRegistry.h>
#include <Script/ScriptBindings.h>

#include <filesystem>


namespace fs = std::filesystem;

EngineContext::EngineContext(int width, int height, const std::string& title)
    // 1. 最先初始化 Logger 與 GLFW 全域狀態庫
    : logger(std::make_unique<LoggerSystem>(65535)),
      glfw(std::make_unique<GlfwContext>()),
      eventBus(std::make_unique<EventBus>()),

    // 2. 建立視窗並使 OpenGL Context Active
      window(std::make_unique<WindowManager>(width, height, title, *eventBus, *logger))
{
    logger->info("[EngineContext] 硬體視窗與核心總線建立完成。");

    // 3. 載入 GLAD 與設定預設混合模式 (必須在 Window 建立後呼叫)
    GlfwContext::initGLAD();
    logger->info("[EngineContext] OpenGL 驅動函式載入 (GLAD) 完成。");

    // 4. 初始化音訊、設定與腳本虛擬機
    audio = std::make_unique<AudioSystem>();
    settings = std::make_unique<SettingsRegistry>(*logger);
    scriptEngine = std::make_unique<ScriptEngine>();
    console = std::make_unique<ConsoleSystem>(*logger);
    console->setScriptEngine(scriptEngine.get());

    // 5. 初始化鍵盤事件監聽器 (向 eventBus 註冊)
    keyboard = std::make_unique<KeyboardSystem>(*eventBus);

    // 6. 資源管理器就緒 (準備加載 Shader 與貼圖)
    resources = std::make_unique<ResourceManager>(*logger);

	// 註冊 Lua C++ 介面
    ScriptBindings::registerAll(
        scriptEngine->getRawState(),
        *logger,
        *audio,
        *settings
    );

    logger->info("[EngineContext] 所有核心子系統構造好了。");
}

EngineContext::~EngineContext() {
    if (logger) {
        logger->info("[EngineContext] 開始依照逆序釋放底層子系統...");
    }

    // 顯式按照嚴格的相依性逆序釋放，雙重保險避免驅動未定義行為：

    // 1. 先清空資源（刪除 Shader Program、Font Texture 等 GPU 句柄）
    resources.reset();

    // 2. 釋放鍵盤系統與控制台（中斷對 EventBus 與 ScriptEngine 的參考）
    keyboard.reset();
    console.reset();

    // 3. 關閉 Lua 虛擬機（此時 audio 與 settings 仍存活，Lua gc 不會踩空）
    scriptEngine.reset();
    settings.reset();

    // 4. 關閉音訊引擎執行緒
    audio.reset();

    // 5. 銷毀 GLFWwindow（銷毀當前 OpenGL Context）
    window.reset();

    // 6. 銷毀事件總線
    eventBus.reset();

    // 7. 呼叫 glfwTerminate()
    glfw.reset();

    // 8. 最後關閉 Logger
    if (logger) {
        logger->info("[EngineContext] 底層子系統已完全釋放。");
        logger.reset();
    }
}