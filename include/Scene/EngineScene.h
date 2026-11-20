#ifndef GIRIGARA_SCENE_ENGINE_SCENE_H
#define GIRIGARA_SCENE_ENGINE_SCENE_H

#include <vulkan/vulkan.h>
#include <Scene/Scene.h>
#include <Scene/SceneManager.h>
#include <UI/UIWindow.h>
#include <UI/UIViewport.h>
#include <vector>
#include <memory>

class VulkanContext;
class UIRenderer;

class EngineScene : public Scene {
public:
    EngineScene(CoreContext& ctx, SceneManager& sceneMgr, VulkanContext& vkCtx, UIRenderer& uiRenderer);
    ~EngineScene() override = default;

    void onEnter() override;
    void onExit() override;
    void update(float dt) override;

    std::unique_ptr<UIContainer> createUI() override;

    void renderViewportsOffscreen(VkCommandBuffer cmd);

    UIViewport* openViewportWindow(UIContainer& root, const std::u32string& title,
                                   float x, float y, float width, float height,
                                   SceneID targetSceneId);

private:
    VulkanContext& m_vkCtx;
    UIRenderer& m_uiRenderer;
    SceneID m_hostedGameSceneId = 0;
    std::vector<UIViewport*> m_activeViewports;
};

#endif // GIRIGARA_SCENE_ENGINE_SCENE_H
