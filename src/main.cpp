#include <Girigara.h>

#ifdef GIRIGARA_SERVER
    #include <Scene/LuaScene.h> // 或你的 ServerGameScene
#else
    #include <Scene/MainMenuScene.h>
#endif

int main() {
#ifdef GIRIGARA_SERVER
    // 編譯出 Dedicated Server 執行檔
    Girigara engine(25565, 20);
    // engine.switchScene<LuaScene>("server_main");
    engine.run();
#else
    // 編譯出 Client 執行檔
    Girigara engine(800, 600, "Project Girigara");
	engine.startIntegratedServer(25565, 20); // 在背景線程拉起無頭伺服器並開啟 25565 埠
	engine.connectToServer("127.0.0.1", 25565); // 前端自動連進本機伺服器
    engine.switchScene<MainMenuScene>();
    engine.run();
#endif

    return 0;
}