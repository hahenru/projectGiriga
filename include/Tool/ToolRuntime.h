#ifndef GIRIGARA_TOOL_TOOL_RUNTIME_H
#define GIRIGARA_TOOL_TOOL_RUNTIME_H

#include <memory>
#include <string>
#include <Tool/ToolContext.h>
#include <Scene/SceneManager.h>
#include <UI/UIManager.h>
#include <Shader/Shader.h>

class ToolRuntime {
public:
    ToolRuntime(int width = 960, int height = 640, const std::string& title = "Girigara Asset Packer");
    ~ToolRuntime();

    void run();
    void stop() { m_isRunning = false; }

    ToolContext& getContext() { return *m_context; }
    SceneManager& getSceneManager() { return *m_sceneManager; }

    template <typename T, typename... Args>
    void switchScene(Args&&... args) {
        m_sceneManager->switchScene<T>(std::forward<Args>(args)...);
    }

private:
    void initSubsystems();
    void loadSystemScripts(const std::string& directoryPath);

    std::unique_ptr<ToolContext> m_context;
    Shader* m_uiShader{nullptr};
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<UIManager> m_uiManager;
    bool m_isRunning{false};
};

#endif // GIRIGARA_TOOL_TOOL_RUNTIME_H
