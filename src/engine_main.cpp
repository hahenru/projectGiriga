#include <Girigara.h>
#include <Scene/EngineScene.h>
#include <exception>
#include <iostream>

int main() {
    try {
        GirigaraConfig config;
        config.headless = false;
        config.width = 1600;
        config.height = 900;
        config.title = "Project Girigara Engine (Vulkan)";
        config.enableAudio = true;
        config.enableNetwork = true;
        config.enableDevTools = true;

        Girigara engine(config);

        engine.switchScene<EngineScene>(
            engine.getVulkanContext(),
            engine.getUIManager()->getRenderer()
        );

        engine.run();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal Error] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
