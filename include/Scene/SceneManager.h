#ifndef GIRIGARA_SCENE_SCENE_MANAGER_H
#define GIRIGARA_SCENE_SCENE_MANAGER_H

#include <Scene/Scene.h>
#include <Core/EngineContext.h>
#include <Event/Event.h>
#include <Event/EventBus.h>
#include <memory>
#include <functional>
#include <utility>

class SceneManager {
public:
    explicit SceneManager(EngineContext& ctx) : m_ctx(ctx) {}
    ~SceneManager();

    template <typename T, typename... Args>
    void switchScene(Args&&... args) {
        m_pendingSceneFactory = [this, ...capturedArgs = std::forward<Args>(args)]() mutable {
            if (m_currentScene) {
                m_currentScene->onExit();
            }
            // 永遠只傳 (*this, m_ctx) 和場景專屬參數！
            m_currentScene = std::make_unique<T>(*this, m_ctx, std::move(capturedArgs)...);

            auto sceneUI = m_currentScene->createUI();
            m_ctx.eventBus->publish(SceneUIChangedEvent{ std::move(sceneUI) });

            m_currentScene->onEnter();
        };
    }

    void update(float dt);
    Scene* getCurrentScene() const { return m_currentScene.get(); }

private:
    void applyPendingScene();

    EngineContext& m_ctx;
    std::unique_ptr<Scene> m_currentScene{nullptr};
    std::function<void()> m_pendingSceneFactory{nullptr};
};

#endif // GIRIGARA_SCENE_SCENE_MANAGER_H