#ifndef GIRIGARA_ENGINE_GIRIGARA_H
#define GIRIGARA_ENGINE_GIRIGARA_H

#include <memory>
#include <string>
#include <thread>
#include <cstdint>

#include <Server/ServerRuntime.h>
#ifndef GIRIGARA_SERVER
    #include <Client/ClientRuntime.h>
#endif

class Girigara {
public:
#ifdef GIRIGARA_SERVER
    explicit Girigara(uint16_t port = 25565, int tickRate = 20);
#else
    Girigara(int width = 800, int height = 600, const std::string& title = "ProjectGirigara");
#endif
    ~Girigara();

    void run();

    // 啟動 / 停止背景無頭伺服器（供單人開房 / Listen Server 使用）
    void startIntegratedServer(uint16_t port = 25565, int tickRate = 20);
    void stopIntegratedServer();

    // 讓前端連向指定伺服器（本機 127.0.0.1 或遠端 IP）
#ifndef GIRIGARA_SERVER
    void connectToServer(const std::string& host, uint16_t port = 25565);
    ClientRuntime& client() { return *m_client; }
#endif

    ServerRuntime* server() { return m_server.get(); }

    template <typename T, typename... Args>
    void switchScene(Args&&... args) {
    #ifdef GIRIGARA_SERVER
        m_server->switchScene<T>(std::forward<Args>(args)...);
    #else
        m_client->switchScene<T>(std::forward<Args>(args)...);
    #endif
    }
private:
    std::unique_ptr<ServerRuntime> m_server;
    std::jthread m_serverThread;

#ifndef GIRIGARA_SERVER
    std::unique_ptr<ClientRuntime> m_client;
#endif
};

#endif // GIRIGARA_ENGINE_GIRIGARA_H