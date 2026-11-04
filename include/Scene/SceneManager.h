#ifndef GIRIGARA_SCENE_SCENE_MANAGER_H
#define GIRIGARA_SCENE_SCENE_MANAGER_H

#include <Scene/Scene.h>
#include <Event/EventBus.h>
#include <Event/Event.h>
#include <Resource/ResourceManager.h>
#include <System/LoggerSystem.h>
#include <memory>
#include <functional>
#include <utility>

class SceneManager {
public:
    SceneManager(EventBus& bus, ResourceManager& resources, LoggerSystem& logger);
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager(SceneManager&&) noexcept = default;
    SceneManager& operator=(SceneManager&&) noexcept = default;

    // 模板工廠閉包：排程場景建立，並透過 EventBus 廣播 UI 樹移交
    template <typename T, typename... Args>
    void switchScene(Args&&... args) {
        m_pendingSceneFactory = [this, ...capturedArgs = std::forward<Args>(args)]() mutable {
            if (m_currentScene) {
                m_currentScene->onExit();
            }
            m_currentScene = std::make_unique<T>(*this, m_bus, m_resources, m_logger, std::move(capturedArgs)...);

            // 提取新場景的 UI 樹並透過事件通知 UIManager 接管
            auto sceneUI = m_currentScene->createUI();
            m_bus.publish(SceneUIChangedEvent{ std::move(sceneUI) });

            m_currentScene->onEnter();
        };
    }

    void update(float dt);

    Scene* getCurrentScene() const { return m_currentScene.get(); }

private:
    void applyPendingScene();

    EventBus& m_bus;
    ResourceManager& m_resources;
    LoggerSystem& m_logger;

    std::unique_ptr<Scene> m_currentScene{nullptr};
    std::function<void()> m_pendingSceneFactory{nullptr};
};

#endif // GIRIGARA_SCENE_SCENE_MANAGER_H