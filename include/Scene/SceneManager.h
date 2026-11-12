#ifndef GIRIGARA_SCENE_SCENE_MANAGER_H
#define GIRIGARA_SCENE_SCENE_MANAGER_H

#include <memory>
#include <functional>
#include <Scene/Scene.h>
#include <Core/CoreContext.h>
#include <Event/EventBus.h>

class SceneManager {
public:
    explicit SceneManager(CoreContext& ctx) : m_ctx(ctx) {}
    ~SceneManager();

    void update(float dt);
    void applyPendingScene();

	template<typename T, typename... Args>
	void switchScene(Args&&... args) {
		m_pendingSceneFactory = [this, ...args = std::forward<Args>(args)]() mutable {
			if (m_currentScene) {
				m_currentScene->onExit();
			}
			m_currentScene = std::make_unique<T>(m_ctx, *this, std::forward<Args>(args)...);
			m_currentScene->onEnter();

			// 直接走 EventBus 移交新場景的 UI 樹所有權
			if (m_ctx.eventBus) {
				m_ctx.eventBus->publish(SceneUIChangedEvent{m_currentScene->createUI()});
			}
		};
	}

    Scene* getCurrentScene() const { return m_currentScene.get(); }

private:
    CoreContext& m_ctx;
    std::unique_ptr<Scene> m_currentScene;
    std::function<void()> m_pendingSceneFactory;
};

#endif // GIRIGARA_SCENE_SCENE_MANAGER_H