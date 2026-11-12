#include <Server/ServerRuntime.h>
#include <Resource/ScriptResource.h>
#include <chrono>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

ServerRuntime::ServerRuntime(uint16_t port, int tickRate)
    : m_context(std::make_unique<ServerContext>()),
      m_port(port),
      m_tickRate(tickRate) {
    m_context->logger->info("[Server] 無頭伺服器初始化中 (Port: " + std::to_string(m_port) + ")...");

    loadSystemScripts("./assets/sysScripts");
    m_sceneManager = std::make_unique<SceneManager>(*m_context);

    // 綁定基礎網路事件並啟動 ENet 監聽
    m_context->network->setOnConnect([this](auto&& peer) {
        m_context->logger->info("[Server] 新玩家連線！");
    });
    m_context->network->setOnDisconnect([this](auto&& peer) {
        m_context->logger->info("[Server] 玩家斷線。");
    });
    m_context->network->startServer(m_port, 32);

    m_context->logger->info("[Server] 無頭伺服器就緒。");
}

ServerRuntime::~ServerRuntime() {
    m_sceneManager.reset();
}

void ServerRuntime::loadSystemScripts(const std::string& directoryPath) {
    fs::path dir(directoryPath);
    if (!fs::exists(dir) || !fs::is_directory(dir)) return;

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            std::string filePath = entry.path().string();
            std::string scriptName = entry.path().stem().string();
            m_context->serverResources->loadScript(scriptName, filePath);
            const auto& scriptRes = m_context->serverResources->get<ScriptResource>(scriptName);
            m_context->scriptEngine->executeResource(scriptRes);
        }
    }
}

void ServerRuntime::run() {
    m_isRunning = true;
    const double fixedDt = 1.0 / static_cast<double>(m_tickRate);
    const auto tickDuration = std::chrono::duration<double>(fixedDt);

    while (m_isRunning) {
        auto frameStart = std::chrono::steady_clock::now();

        // 1. 輪詢 ENet 網路封包
        m_context->network->poll(0);

        // 2. 推進無頭場景邏輯
        m_sceneManager->update(static_cast<float>(fixedDt));

        // 3. 精準休眠補足剩餘 Tick 時間，避免 CPU 空轉 100%
        auto elapsed = std::chrono::steady_clock::now() - frameStart;
        if (elapsed < tickDuration) {
            std::this_thread::sleep_for(tickDuration - elapsed);
        }
    }
}

void ServerRuntime::startNetwork(size_t maxClients) {
    if (m_context->network) {
        m_context->network->startServer(m_port, maxClients);
    }
}