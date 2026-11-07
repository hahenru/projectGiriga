#include <Scene/GameScene.h>
#include <Scene/SceneManager.h>
#include <Scene/MainMenuScene.h>
#include <UI/UIButton.h>
#include <UI/UIText.h>
#include <iostream>

std::unique_ptr<UIContainer> GameScene::createUI() {
    auto uiRoot = std::make_unique<UIContainer>();

    // 放置一個在父容器水平 50.0%、垂直 80.0% 的按鈕
    auto* btn = uiRoot->emplaceChild<UIButton>(0.0f, 0.0f, 200.0f, 50.0f);
    btn->setPositionPercent(50.0f, 80.0f);

    // 精確座標文字：改傳 m_resources
    auto* text = uiRoot->emplaceChild<UIText>(0.0f, 0.0f, m_resources, U"精確座標");
    text->setPositionPercent(12.34f, 56.78f);

    // 遊戲場景專屬的對話視窗：改傳 m_resources
    m_consoleWindow = uiRoot->emplaceChild<UIWindow>(
        180.0f, 100.0f, 440.0f, 260.0f,
        m_resources, U"測試用視窗"
    );
    m_consoleWindow->setCloseOnEsc(true);

    auto* statusText = m_consoleWindow->emplaceContent<UIText>(20.0f, 50.0f, m_resources, U"請輸入文字");
    statusText->setColor(0.3f, 0.9f, 0.4f, 1.0f);

    // 回到主選單按鈕
    auto* btnBack = uiRoot->emplaceChild<UIButton>(50.0f, 500.0f, 180.0f, 50.0f);
    btnBack->setColors({
        .normal  = {0.4f, 0.4f, 0.45f, 1.0f},
        .hovered = {0.5f, 0.5f, 0.55f, 1.0f},
        .pressed = {0.3f, 0.3f, 0.35f, 1.0f}
    });
    btnBack->setOnClick([this]() {
        m_manager.switchScene<MainMenuScene>();
    });

    auto* backLabel = uiRoot->emplaceChild<UIText>(0.0f, 0.0f, m_resources, U"回主選單");
    backLabel->setPosition(btnBack->getX() + 20.0f, btnBack->getY() + 10.0f);

    return uiRoot;
}

void GameScene::onEnter() {
    std::cout << "[Scene] Entering GameScene...\n";
}

void GameScene::onExit() {
    std::cout << "[Scene] Exiting GameScene (Game entities & console window destroyed).\n";
}
