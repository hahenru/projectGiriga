#ifndef GIRIGARA_UI_UI_VIEWPORT_H
#define GIRIGARA_UI_UI_VIEWPORT_H

#include <UI/UIElement.h>
#include <Renderer/VulkanRenderTarget.h>
#include <Scene/SceneManager.h>
#include <memory>

class UIRenderer;

class UIViewport : public UIElement {
public:
    UIViewport(float x, float y, float width, float height,
               VulkanContext& vkCtx, UIRenderer& renderer, SceneID targetScene = 0);
    ~UIViewport() override = default;

    void setTargetScene(SceneID sceneId) { m_targetScene = sceneId; }
    SceneID getTargetScene() const { return m_targetScene; }

    void renderSceneOffscreen(VkCommandBuffer cmd, SceneManager& sceneManager);
    void render(UIRenderer& renderer) override;

    bool handleMouseMove(double x, double y) override;
    bool handleMouseButton(int button, int action, int mods, double x, double y) override;

private:
    SceneID m_targetScene = 0;
    std::unique_ptr<VulkanRenderTarget> m_renderTarget;
};

#endif // GIRIGARA_UI_UI_VIEWPORT_H
