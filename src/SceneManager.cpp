#include <Scene/SceneManager.h>

SceneManager::~SceneManager() {
    if (m_currentScene) {
        m_currentScene->onExit();
        m_currentScene.reset();
    }
}

void SceneManager::update(float dt) {
    applyPendingScene();
    if (m_currentScene) {
        m_currentScene->update(dt);
    }
}

void SceneManager::applyPendingScene() {
    if (m_pendingSceneFactory) {
        auto factory = std::move(m_pendingSceneFactory);
        m_pendingSceneFactory = nullptr;
        factory();
    }
}