#include <Girigara.h>
#include <Server/ServerConfig.h>

#if defined(GIRIGARA_SERVER)
#include <Scene/LuaScene.h>
#else
#include <Scene/MainMenuScene.h>
#endif

int main() {
#if defined(GIRIGARA_SERVER)
    auto cfg = ServerConfig::loadOrCreate("server.cfg");
    Girigara engine({
        .headless = true,
        .tickRate = cfg.tickRate,
        .enableNetwork = true
    });
    engine.startServer(cfg.port, cfg.maxClients);
    engine.run();
#else
    Girigara engine({
        .headless = false,
        .width = 800,
        .height = 600,
        .title = "Project Girigara",
        .enableAudio = true,
        .enableNetwork = true,
        .enableDevTools = true
    });
    engine.connectToServer("127.0.0.1", 25565);
    engine.switchScene<MainMenuScene>();
    engine.run();
#endif
    return 0;
}