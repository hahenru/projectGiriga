#include <Scene/SceneManager.h>
#include <Audio/AudioSystem.h>

SceneManager::SceneManager(EventBus& bus, ResourceManager& resources, LoggerSystem& logger, AudioSystem& audio)
    : m_bus(bus), m_resources(resources), m_logger(logger), m_audio(audio) {
}

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