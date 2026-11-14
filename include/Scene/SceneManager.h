#ifndef GIRIGARA_SCENE_SCENE_MANAGER_H
#define GIRIGARA_SCENE_SCENE_MANAGER_H

#include <memory>
#include <functional>
#include <string>
#include <Core/Identifiable.h>
#include <Core/CoreContext.h>
#include <Scene/Scene.h>
#include <Event/EventBus.h>

class SceneManager : public Identifiable {
public:
	explicit SceneManager(CoreContext& ctx);
	~SceneManager() override;

	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	void update(float dt);
	void applyPendingScene();

	// 供 Lua 腳本或字串指令呼叫的便利介面（切換至指定腳本資源的 LuaScene）
	void switchToLuaScene(const std::string& scriptResourceName);

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
	void registerScriptBindings();
	void unregisterScriptBindings();

	CoreContext& m_ctx;
	std::unique_ptr<Scene> m_currentScene;
	std::function<void()> m_pendingSceneFactory;
};

#endif // GIRIGARA_SCENE_SCENE_MANAGER_H
