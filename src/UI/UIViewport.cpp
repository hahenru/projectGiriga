#include <UI/UIViewport.h>
#include <Renderer/UIRenderer.h>
#include <Scene/SceneManager.h>
#include <algorithm>

UIViewport::UIViewport(float x, float y, float width, float height,
                       VulkanContext& vkCtx, UIRenderer& renderer, SceneID targetScene)
    : UIElement(x, y, width, height), m_targetScene(targetScene) {
    m_renderTarget = std::make_unique<VulkanRenderTarget>(
        vkCtx,
        renderer.getDescriptorPool(),
        renderer.getDescriptorSetLayout(),
        static_cast<int>(std::max(1.0f, width)),
        static_cast<int>(std::max(1.0f, height))
    );
}

void UIViewport::renderSceneOffscreen(VkCommandBuffer cmd, SceneManager& sceneManager) {
    if (!m_visible || !m_renderTarget || cmd == VK_NULL_HANDLE) return;

    int targetW = static_cast<int>(std::max(1.0f, getLocalWidth()));
    int targetH = static_cast<int>(std::max(1.0f, getLocalHeight()));
    m_renderTarget->resize(targetW, targetH);

    // Pass 1：將指定 Scene 渲染進本視口的離屏 VkImage
    m_renderTarget->beginRendering(cmd, 0.07f, 0.09f, 0.13f);

    if (m_targetScene != 0) {
        if (Scene* scene = sceneManager.getScene(m_targetScene)) {
            // 未來 Scene::renderWorld(cmd) 接通後在此錄製場景指令
            scene->renderWorld();
        }
    }

    m_renderTarget->endRendering(cmd);
}

void UIViewport::render(UIRenderer& renderer) {
    if (!m_visible || !m_renderTarget) return;

    float gx = getGlobalX();
    float gy = getGlobalY();
    float w = getLocalWidth();
    float h = getLocalHeight();

    // Pass 2：在 UIWindow 內容區直接將離屏畫布作為 RGBA 貼圖繪製出來
    renderer.drawTexture(gx, gy, w, h, m_renderTarget->getTexture(), 1.0f, 1.0f, 1.0f, 1.0f, 2);
}

bool UIViewport::handleMouseMove(double x, double y) {
    if (!m_visible) return false;
    return contains(x, y);
}

bool UIViewport::handleMouseButton(int /*button*/, int /*action*/, int /*mods*/, double x, double y) {
    if (!m_visible) return false;
    // 點擊落在遊戲視口內時攔截事件，後續可在此將局部座標 (x - getGlobalX(), y - getGlobalY()) 轉發給目標 Scene
    return contains(x, y);
}