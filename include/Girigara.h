#ifndef GIRIGARA_ENGINE_GIRIGARA_H
#define GIRIGARA_ENGINE_GIRIGARA_H

#include <memory>
#include <string>
#include <functional>

#include <System/LoggerSystem.h>
#include <System/ConsoleSystem.h>
#include <Window/GlfwContext.h>
#include <Window/WindowManager.h>
#include <Event/EventBus.h>
#include <System/KeyboardSystem.h>
#include <Resource/ResourceManager.h>
#include <Script/ScriptEngine.h>
#include <Scene/SceneManager.h>
#include <Renderer/QuadMesh.h>
#include <Shader/Shader.h>
#include <UI/UIConsole.h>
#include <UI/UIWindow.h>
#include <Scene/LuaScene.h>
#include <UI/UIManager.h>
#include <Audio/AudioSystem.h>

class Girigara {
public:
    Girigara(int width = 800, int height = 600, const std::string& title = "ProjectGirigara");
    ~Girigara() = default;

    void run();

    LoggerSystem& getLogger() { return m_logger; }
    ConsoleSystem& getConsole() { return m_console; }
    EventBus& getEventBus() { return m_eventBus; }
    WindowManager& getWindow() { return *m_window; }
    ResourceManager& getResources() { return m_resources; }
    ScriptEngine& getScriptEngine() { return m_scriptEngine; }
    SceneManager& getSceneManager() { return *m_sceneManager; }

    template <typename T, typename... Args>
    void switchScene(Args&&... args) {
        m_sceneManager->switchScene<T>(std::forward<Args>(args)...);
    }
    // 2. 新增便捷介面：一行指令直接切換至指定的 Lua 腳本場景
    void switchLuaScene(const std::string& scriptResourceName) {
        // SceneManager 會自動補齊前 4 個參數，這裡只需傳入 ScriptEngine 和 資源名稱
        m_sceneManager->switchScene<LuaScene>(
            std::ref(m_scriptEngine),
            scriptResourceName
        );
    }

private:
    void initSubsystems();
    void loadSystemScripts(const std::string& directoryPath); // ★ 掃描並載入資料夾內所有腳本
    void setupUI();
    void setupShortcuts();

    // RAII 宣告順序：LoggerSystem 位於最頂部
    LoggerSystem m_logger;
    GlfwContext m_glfw;
    EventBus m_eventBus;
    KeyboardSystem m_keyboardSystem;
    std::unique_ptr<WindowManager> m_window;
    ResourceManager m_resources;
	AudioSystem m_audio;
    ScriptEngine m_scriptEngine;
    ConsoleSystem m_console;
    std::unique_ptr<SceneManager> m_sceneManager;

    Shader* m_uiShader{nullptr};
    std::unique_ptr<QuadMesh> m_quadMesh;

    UIConsole* m_inGameConsole{nullptr};
    UIWindow* m_globalConsoleWindow{nullptr};
	std::unique_ptr<UIManager> m_uiManager{nullptr};

    bool m_isRunning{false};
};

#endif // GIRIGARA_ENGINE_GIRIGARA_H
