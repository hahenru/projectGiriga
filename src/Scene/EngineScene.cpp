#include <Scene/EngineScene.h>
#include <Scene/GameScene.h>
#include <Core/CoreContext.h>
#include <Renderer/UIRenderer.h>
#include <System/LoggerSystem.h>
#include <algorithm>

EngineScene::EngineScene(CoreContext& ctx, SceneManager& sceneMgr, VulkanContext& vkCtx, UIRenderer& uiRenderer)
: Scene(ctx, sceneMgr), m_vkCtx(vkCtx), m_uiRenderer(uiRenderer) {
    setName("EngineWorkspace");
}

void EngineScene::onEnter() {
    m_ctx.logger->info("[EngineScene] 進入引擎工作區模式...");

    // 在背景掛載目標遊戲場景（以 GameScene 為預設專案測試場景）
    m_hostedGameSceneId = m_manager.mountScene<GameScene>(
        "LiveGameViewport",
        SceneExecutionMode::MainThread,
        60
    );
    m_ctx.logger->info("[EngineScene] 已在背景掛載目標遊戲場景，SceneID = " + std::to_string(m_hostedGameSceneId));
}

void EngineScene::onExit() {
    m_activeViewports.clear();
    if (m_hostedGameSceneId != 0) {
        m_manager.unmountScene(m_hostedGameSceneId);
        m_hostedGameSceneId = 0;
    }
}

void EngineScene::update(float /*dt*/) {
    // 未來可在此處理編輯器快捷鍵、工具列狀態同步或 Gizmo 操作
}

std::unique_ptr<UIContainer> EngineScene::createUI() {
    const float screenW = m_uiRenderer.getScreenWidth();
    const float screenH = m_uiRenderer.getScreenHeight();

    auto root = std::make_unique<UIContainer>(0.0f, 0.0f, screenW, screenH);
    root->setPassThrough(true);

    // 建立主遊戲視口視窗，置中偏右配置，預留左側與下方給未來屬性面板與 Console
    const float winW = std::min(960.0f, screenW * 0.72f);
    const float winH = std::min(575.0f, screenH * 0.75f);
    const float winX = (screenW - winW) * 0.5f;
    const float winY = (screenH - winH) * 0.45f;

    openViewportWindow(*root, U"Game Viewport [Live Scene]", winX, winY, winW, winH, m_hostedGameSceneId);

    return root;
}

UIViewport* EngineScene::openViewportWindow(UIContainer& root, const std::u32string& title,
                                            float x, float y, float width, float height,
                                            SceneID targetSceneId) {
    constexpr float kTitleBarHeight = 35.0f;

    auto gameWin = std::make_unique<UIWindow>(
        x, y, width, height,
        *m_ctx.resources, title, 18
    );
    gameWin->setCloseOnEsc(false);

    auto* viewport = gameWin->emplaceContent<UIViewport>(
        0.0f, kTitleBarHeight,
        width, std::max(1.0f, height - kTitleBarHeight),
                                                         m_vkCtx, m_uiRenderer, targetSceneId
    );

    m_activeViewports.push_back(viewport);
    root.addChild(std::move(gameWin));
    return viewport;
                                            }

                                            void EngineScene::renderViewportsOffscreen(VkCommandBuffer cmd) {
                                                for (UIViewport* vp : m_activeViewports) {
                                                    if (vp && vp->isVisible()) {
                                                        vp->renderSceneOffscreen(cmd, m_manager);
                                                    }
                                                }
                                            }
