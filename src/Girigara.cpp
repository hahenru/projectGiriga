#include <Girigara.h>

#if defined(GIRIGARA_SERVER)
#include <Server/ServerConfig.h>

Girigara::Girigara(const std::string& configPath) {
    auto cfg = ServerConfig::loadOrCreate(configPath);
    m_server = std::make_unique<ServerRuntime>(cfg.port, cfg.tickRate);
    m_server->startNetwork(cfg.maxClients);
}

Girigara::Girigara(uint16_t port, int tickRate)
: m_server(std::make_unique<ServerRuntime>(port, tickRate)) {
    // 手動指定埠號模式（預設最大 32 人）
    m_server->startNetwork(32);
}

Girigara::~Girigara() {
    if (m_server) m_server->stop();
}

void Girigara::run() {
    m_server->run();
}

#elif defined(GIRIGARA_TOOL)

Girigara::Girigara(int width, int height, const std::string& title)
: m_tool(std::make_unique<ToolRuntime>(width, height, title)) {}

Girigara::~Girigara() {
    m_tool.reset();
}

void Girigara::run() {
    m_tool->run();
}

#else

Girigara::Girigara(int width, int height, const std::string& title)
: m_client(std::make_unique<ClientRuntime>(width, height, title)) {}

Girigara::~Girigara() {
    #ifndef GIRIGARA_NO_INTEGRATED_SERVER
    stopIntegratedServer();
    #endif
    m_client.reset();
}

#ifndef GIRIGARA_NO_INTEGRATED_SERVER
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
#endif

void Girigara::connectToServer(const std::string& host, uint16_t port) {
    if (m_client && m_client->getContext().network) {
        m_client->getContext().network->connectClient(host, port);
    }
}

void Girigara::run() {
    m_client->run();
}

#endif
