#ifndef GIRIGARA_ENGINE_GIRIGARA_H
#define GIRIGARA_ENGINE_GIRIGARA_H

#include <memory>
#include <string>
#include <functional>

#include <Core/EngineContext.h>

#include <Scene/SceneManager.h>
#include <UI/UIManager.h>
#include <Renderer/QuadMesh.h>
#include <Shader/Shader.h>

class Girigara {
public:
    Girigara(int width = 800, int height = 600, const std::string& title = "ProjectGirigara");
    ~Girigara();

    void run();

    // 透過 Context 代理取得底層系統
    EngineContext& getContext() { return *m_context; }
    LoggerSystem& getLogger() { return *m_context->logger; }
    EventBus& getEventBus() { return *m_context->eventBus; }
    WindowManager& getWindow() { return *m_context->window; }
    ResourceManager& getResources() { return *m_context->resources; }
    ScriptEngine& getScriptEngine() { return *m_context->scriptEngine; }
    SceneManager& getSceneManager() { return *m_sceneManager; }

    template <typename T, typename... Args>
    void switchScene(Args&&... args) {
        m_sceneManager->switchScene<T>(std::forward<Args>(args)...);
    }

private:
    void initSubsystems();
    void loadSystemScripts(const std::string& directoryPath);
    void setupShortcuts();

    // 1. 底層所有硬體與服務上下文 (最先建構，最後解構)
    std::unique_ptr<EngineContext> m_context;

    // 2. 運行時 UI 與場景層 (依賴 m_context)
    Shader* m_uiShader{nullptr};
    std::unique_ptr<QuadMesh> m_quadMesh;
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<UIManager> m_uiManager;

    bool m_isRunning{false};
};

#endif // GIRIGARA_ENGINE_GIRIGARA_H
