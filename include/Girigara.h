// include/Girigara.h
#ifndef GIRIGARA_ENGINE_GIRIGARA_H
#define GIRIGARA_ENGINE_GIRIGARA_H

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <cstdint>

#include <Core/CoreContext.h>
#include <Scene/SceneManager.h>
#include <Network/ENetContext.h>
#include <Network/ENetNode.h>
#include <Server/ServerResourceManager.h>

#ifndef GIRIGARA_SERVER
#include <Window/GlfwContext.h>
#include <Renderer/VulkanContext.h>
#include <Window/WindowManager.h>
#include <System/KeyboardSystem.h>
#include <Client/ClientResourceManager.h>
#include <Audio/AudioSystem.h>
#include <UI/UIManager.h>
#include <UI/UIConsole.h>
#include <UI/UISettingsMenu.h>
#endif

struct GirigaraConfig {
    bool headless = false;              // true = 無頭伺服器模式
    int width = 1280;
    int height = 720;
    std::string title = "Project Girigara";
    int tickRate = 20;                  // 固定邏輯更新頻率 (Hz)
    bool enableAudio = true;
    bool enableNetwork = true;
    bool enableDevTools = true;         // 內建開發者 Console 與系統設定視窗
    std::string serverConfigPath = "server.cfg"; // 無頭模式自動讀取的設定檔
};

class Girigara {
public:
    explicit Girigara(const GirigaraConfig& config = GirigaraConfig{});
    ~Girigara();

    Girigara(const Girigara&) = delete;
    Girigara& operator=(const Girigara&) = delete;

    // 啟動引擎主迴圈
    void run();
    void stop() { m_isRunning = false; }

    // 網路控制
    bool startServer(uint16_t port = 25565, size_t maxClients = 32);
    bool connectToServer(const std::string& host, uint16_t port = 25565);

#ifndef GIRIGARA_SERVER
    // 單機內建無頭伺服器分身（共用同一套 Girigara 引擎）
    void startIntegratedServer(uint16_t port = 25565, int tickRate = 20);
    void stopIntegratedServer();
#endif

    // 系統腳本載入
    void loadSystemScripts(const std::string& directoryPath = "./assets/sysScripts");

    // 場景切換與掛載
    template <typename T, typename... Args>
    void switchScene(Args&&... args) {
        m_sceneManager->switchScene<T>(std::forward<Args>(args)...);
    }

    // 核心子系統存取（給各個 main 進入點或測試程式直接呼叫）
    CoreContext& getContext() { return *m_context; }
    EventBus& getEventBus() { return *m_context->eventBus; }
    LoggerSystem& getLogger() { return *m_context->logger; }
    ConsoleSystem& getConsole() { return *m_context->console; }
    FileSystem& getFileSystem() { return *m_context->fileSystem; }
    ScriptEngine& getScriptEngine() { return *m_context->scriptEngine; }
    SceneManager& getSceneManager() { return *m_sceneManager; }
    IResourceManager& getResources() { return *m_context->resources; }
    ENetNode* getNetwork() { return m_network.get(); }

#ifndef GIRIGARA_SERVER
    SettingsRegistry& getSettings() { return *m_context->settings; }
    VulkanContext& getVulkanContext() { return *m_vkCtx; }
    WindowManager* getMainWindow() { return m_windows.empty() ? nullptr : m_windows.front().get(); }
    UIManager* getUIManager() { return m_uiManager.get(); }
    AudioSystem* getAudio() { return m_audio.get(); }
    KeyboardSystem* getKeyboard() { return m_keyboard.get(); }
#endif

private:
    void initHeadlessSubsystems();
    void initGraphicalSubsystems();
    void initDevTools(float width, float height);
    void runHeadlessLoop();
    void runGraphicalLoop();

    GirigaraConfig m_config;
    bool m_isRunning{false};

    // 1. 全端共用核心與場景管理器
    std::unique_ptr<CoreContext> m_context;
    std::unique_ptr<ServerResourceManager> m_serverResources;
    std::unique_ptr<SceneManager> m_sceneManager;

    // 2. 網路層
    std::unique_ptr<ENetContext> m_enetCtx;
    std::unique_ptr<ENetNode> m_network;

#ifndef GIRIGARA_SERVER
    // 3. 圖形、多視窗預留、輸入、音訊與 UI 層
    std::unique_ptr<GlfwContext> m_glfw;
	std::unique_ptr<VulkanContext> m_vkCtx;
    std::vector<std::unique_ptr<WindowManager>> m_windows;
    std::unique_ptr<KeyboardSystem> m_keyboard;
    std::unique_ptr<AudioSystem> m_audio;
    std::unique_ptr<ClientResourceManager> m_clientResources;
    std::unique_ptr<UIManager> m_uiManager;

    // 內建開發者工具（非擁有指標，生命週期由 UIManager::m_overlayUI 管理）
    UIConsole* m_devConsole{nullptr};
    UISettingsMenu* m_devSettingsMenu{nullptr};

    // 單機開房的內建無頭引擎分身
    std::unique_ptr<Girigara> m_integratedServer;
    std::jthread m_integratedServerThread;
#endif
};

#endif // GIRIGARA_ENGINE_GIRIGARA_H
