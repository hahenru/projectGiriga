#ifndef GIRIGARA_ENGINE_GIRIGARA_H
#define GIRIGARA_ENGINE_GIRIGARA_H

#include <memory>
#include <string>
#include <thread>
#include <cstdint>

#if defined(GIRIGARA_SERVER)
#include <Server/ServerRuntime.h>
#elif defined(GIRIGARA_TOOL)
#include <Tool/ToolRuntime.h>
#else
#include <Client/ClientRuntime.h>
#ifndef GIRIGARA_NO_INTEGRATED_SERVER
#include <Server/ServerRuntime.h>
#endif
#endif

class Girigara {
public:
    #if defined(GIRIGARA_SERVER)
    explicit Girigara(const std::string& configPath = "server.cfg");
    Girigara(uint16_t port, int tickRate = 20);
    #elif defined(GIRIGARA_TOOL)
    Girigara(int width = 960, int height = 640, const std::string& title = "Girigara Asset Packer");
    #else
    Girigara(int width = 800, int height = 600, const std::string& title = "ProjectGirigara");
    #endif
    ~Girigara();

    void run();

    #if defined(GIRIGARA_SERVER)
    ServerRuntime* server() { return m_server.get(); }
    #elif defined(GIRIGARA_TOOL)
    ToolRuntime& tool() { return *m_tool; }
    #else
    void connectToServer(const std::string& host, uint16_t port = 25565);
    ClientRuntime& client() { return *m_client; }

    #ifndef GIRIGARA_NO_INTEGRATED_SERVER
    void startIntegratedServer(uint16_t port = 25565, int tickRate = 20);
    void stopIntegratedServer();
    ServerRuntime* server() { return m_server.get(); }
    #endif
    #endif

    template <typename T, typename... Args>
    void switchScene(Args&&... args) {
        #if defined(GIRIGARA_SERVER)
        m_server->switchScene<T>(std::forward<Args>(args)...);
        #elif defined(GIRIGARA_TOOL)
        m_tool->switchScene<T>(std::forward<Args>(args)...);
        #else
        m_client->switchScene<T>(std::forward<Args>(args)...);
        #endif
    }

private:
    #if defined(GIRIGARA_SERVER)
    std::unique_ptr<ServerRuntime> m_server;
    #elif defined(GIRIGARA_TOOL)
    std::unique_ptr<ToolRuntime> m_tool;
    #else
    #ifndef GIRIGARA_NO_INTEGRATED_SERVER
    std::unique_ptr<ServerRuntime> m_server;
    std::jthread m_serverThread;
    #endif
    std::unique_ptr<ClientRuntime> m_client;
    #endif
};

#endif // GIRIGARA_ENGINE_GIRIGARA_H
