#ifndef GIRIGARA_CLIENT_CLIENT_RUNTIME_H
#define GIRIGARA_CLIENT_CLIENT_RUNTIME_H

#include <memory>
#include <string>
#include <Client/ClientContext.h>
#include <Scene/SceneManager.h>
#include <UI/UIManager.h>
#include <Shader/Shader.h>

class ClientRuntime {
public:
    ClientRuntime(int width = 800, int height = 600, const std::string& title = "ProjectGirigara");
    ~ClientRuntime();

    void run();
    void stop() { m_isRunning = false; }

    ClientContext& getContext() { return *m_context; }
    SceneManager& getSceneManager() { return *m_sceneManager; }

    template <typename T, typename... Args>
    void switchScene(Args&&... args) {
        m_sceneManager->switchScene<T>(std::forward<Args>(args)...);
    }

private:
    void initSubsystems();
    void loadSystemScripts(const std::string& directoryPath);

    std::unique_ptr<ClientContext> m_context;
    Shader* m_uiShader{nullptr};
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<UIManager> m_uiManager;
    bool m_isRunning{false};
};

#endif // GIRIGARA_CLIENT_CLIENT_RUNTIME_H