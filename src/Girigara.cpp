#include <Girigara.h>

#ifdef GIRIGARA_SERVER

Girigara::Girigara(uint16_t port, int tickRate)
    : m_server(std::make_unique<ServerRuntime>(port, tickRate)) {
    // 獨立伺服器模式：直接啟動網路監聽
    m_server->startNetwork();
}

Girigara::~Girigara() {
    if (m_server) m_server->stop();
}

void Girigara::run() {
    m_server->run();
}

#else

Girigara::Girigara(int width, int height, const std::string& title)
    : m_client(std::make_unique<ClientRuntime>(width, height, title)) {}

Girigara::~Girigara() {
    stopIntegratedServer();
    m_client.reset();
}

void Girigara::startIntegratedServer(uint16_t port, int tickRate) {
    if (m_server) return; // 已經在運行中

    m_server = std::make_unique<ServerRuntime>(port, tickRate);
    m_server->startNetwork();

    // 開背景執行緒跑伺服器固定 Tick 迴圈
    m_serverThread = std::jthread([this]() {
        m_server->run();
    });
}

void Girigara::stopIntegratedServer() {
    if (m_server) {
        m_server->stop();
        if (m_serverThread.joinable()) {
            m_serverThread.join();
        }
        m_server.reset();
    }
}

void Girigara::connectToServer(const std::string& host, uint16_t port) {
    if (m_client && m_client->getContext().network) {
        m_client->getContext().network->connectClient(host, port);
    }
}

void Girigara::run() {
    m_client->run();
}

#endif