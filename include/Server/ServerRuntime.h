#ifndef GIRIGARA_SERVER_SERVER_RUNTIME_H
#define GIRIGARA_SERVER_SERVER_RUNTIME_H

#include <memory>
#include <string>
#include <atomic>
#include <cstdint>
#include <Server/ServerContext.h>
#include <Scene/SceneManager.h>

class ServerRuntime {
public:
    explicit ServerRuntime(uint16_t port = 25565, int tickRate = 20);
    ~ServerRuntime();

    void run();
    void stop() { m_isRunning = false; }

    ServerContext& getContext() { return *m_context; }
    SceneManager& getSceneManager() { return *m_sceneManager; }

    template <typename T, typename... Args>
    void switchScene(Args&&... args) {
        m_sceneManager->switchScene<T>(std::forward<Args>(args)...);
    }
	void startNetwork(size_t maxClients = 3);

private:
    void loadSystemScripts(const std::string& directoryPath);

    std::unique_ptr<ServerContext> m_context;
    std::unique_ptr<SceneManager> m_sceneManager;
    uint16_t m_port;
    int m_tickRate;
    std::atomic<bool> m_isRunning{false};
};

#endif // GIRIGARA_SERVER_SERVER_RUNTIME_H