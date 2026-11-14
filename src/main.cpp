#include <Girigara.h>

#if defined(GIRIGARA_SERVER)
#include <Scene/LuaScene.h>
#elif defined(GIRIGARA_TOOL)
#include <Scene/LuaScene.h>
#else
#include <Scene/MainMenuScene.h>
#endif

int main() {
    #if defined(GIRIGARA_SERVER)
    // 編譯出 Dedicated Server 執行檔
    Girigara engine;
    engine.run();

    #elif defined(GIRIGARA_TOOL)
    // 獨立編譯出 GirigaraPacker 圖形介面工具（不啟動伺服器也不載入遊戲場景）
    Girigara engine(960, 640, "Girigara Asset Packer (.imz / .luz / .dtz)");
    engine.tool().getContext().toolResources->loadScript("packer_tool", "./assets/scripts/packer_tool.lua");
    engine.switchScene<LuaScene>("packer_tool");
    engine.run();

    #else
    // 編譯出 Client 執行檔
    Girigara engine(800, 600, "Project Girigara");
    // engine.startIntegratedServer(25565, 20); // 在背景線程拉起無頭伺服器並開啟 25565 埠
    engine.connectToServer("127.0.0.1", 25565); // 前端自動連進本機伺服器
    engine.switchScene<MainMenuScene>();
    engine.run();
    #endif

    return 0;
}
