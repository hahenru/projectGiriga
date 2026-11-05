#include <Scene/MainMenuScene.h>
#include <Scene/SceneManager.h>
#include <Scene/GameScene.h>
#include <UI/UIButton.h>
#include <UI/UIText.h>
#include <UI/UISlantedLayout.h>
#include <Audio/AudioSystem.h>
#include <iostream>

std::unique_ptr<UIContainer> MainMenuScene::createUI() {
    auto uiRoot = std::make_unique<UIContainer>();

    // 放置一個在父容器水平 80.0%、垂直 95.0% 的按鈕
    auto* btn = uiRoot->emplaceChild<UIButton>(0.0f, 0.0f, 200.0f, 50.0f);
    btn->setPositionPercent(80.0f, 95.0f);

    // 精確座標文字：改傳 m_resources
    auto* text = uiRoot->emplaceChild<UIText>(0.0f, 0.0f, m_resources, U"精確座標");
    text->setPositionPercent(80.0f, 90.0f);

    // 場景專屬標題：改傳 m_resources
    auto* title = uiRoot->emplaceChild<UIText>(300.0f, 150.0f, m_resources, U"主選單");
    title->setColor(0.95f, 0.85f, 0.20f, 1.0f);

    // 開始遊戲按鈕：點擊後切換到 GameScene
    auto* btnStart = uiRoot->emplaceChild<UIButton>(300.0f, 260.0f, 200.0f, 60.0f);
    btnStart->setOnClick([this]() {
        m_manager.switchScene<GameScene>();
    });

    auto* startLabel = uiRoot->emplaceChild<UIText>(0.0f, 0.0f, m_resources, U"開始");
    startLabel->setColor(1.0f, 1.0f, 1.0f, 1.0f);
    float textX = btnStart->getX() + (btnStart->getWidth() - startLabel->getWidth()) * 0.5f;
    float textY = btnStart->getY() + (btnStart->getHeight() - startLabel->getHeight()) * 0.5f;
    startLabel->setPosition(textX, textY);

    // 建立斜向選單
    auto* menu = uiRoot->emplaceChild<UISlantedLayout>(50.0f, 50.0f, 0.0f, 0.0f);
    menu->setPositionPercent(60.0f, 20.0f);
    menu->setSpacing(12.0f);
    menu->setSlantOffsetX(30.0f);

    // 按鈕 setLabel 改傳 m_resources
    auto* btn1 = menu->emplaceChild<UIButton>(0.0f, 0.0f, 160.0f, 40.0f);
    btn1->setLabel(m_resources, U"▶ 物理攻擊");

    auto* btn2 = menu->emplaceChild<UIButton>(0.0f, 0.0f, 160.0f, 40.0f);
    btn2->setLabel(m_resources, U"▶ 戰術魔法");

    auto* btn3 = menu->emplaceChild<UIButton>(0.0f, 0.0f, 160.0f, 40.0f);
    btn3->setLabel(m_resources, U"▶ 物品欄位");

    return uiRoot;
}

void MainMenuScene::onEnter() {
	m_audio.playBGM("./assets/Sounds/Deserted-Hell.wav", true);
    std::cout << "[Scene] Entering MainMenuScene...\n";
}

void MainMenuScene::onExit() {
    std::cout << "[Scene] Exiting MainMenuScene (UI lifecycle handed over to UIManager).\n";
}