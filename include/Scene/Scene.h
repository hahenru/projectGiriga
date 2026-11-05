#ifndef GIRIGARA_SCENE_SCENE_H
#define GIRIGARA_SCENE_SCENE_H

#include <Event/EventBus.h>
#include <Resource/ResourceManager.h>
#include <System/LoggerSystem.h>
#include <UI/UIContainer.h>
#include <memory>

class AudioSystem;
class SceneManager;

class Scene {
public:
	Scene(SceneManager& manager, EventBus& bus, ResourceManager& resources,
          LoggerSystem& logger, AudioSystem& audio)
        : m_manager(manager), m_bus(bus), m_resources(resources),
          m_logger(logger), m_audio(audio) {}

    virtual ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;

	// 生命週期勾子（提供空預設實作，避免衍生類別未覆寫而變成抽象類別）
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void update(float dt) { (void)dt; }
    virtual void renderWorld() {}

    // ★ 由子類場景實作，回傳該場景要掛載的 UI 根容器
    virtual std::unique_ptr<UIContainer> createUI() {
        return nullptr;
    }

protected:
    SceneManager& m_manager;
    EventBus& m_bus;
    ResourceManager& m_resources;
    LoggerSystem& m_logger;
	AudioSystem& m_audio;
};

#endif // GIRIGARA_SCENE_SCENE_H