#ifndef GIRIGARA_SCENE_SCENE_H
#define GIRIGARA_SCENE_SCENE_H

#include <Core/EngineContext.h>
#include <UI/UIContainer.h>
#include <memory>

class SceneManager;

class Scene {
public:
    Scene(SceneManager& manager, EngineContext& ctx)
        : m_manager(manager), m_ctx(ctx) {}

    virtual ~Scene() = default;

    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void update(float dt) { (void)dt; }
    virtual void renderWorld() {}

    virtual std::unique_ptr<UIContainer> createUI() {
        return nullptr;
    }

protected:
    SceneManager&  m_manager;
    EngineContext& m_ctx;
};

#endif // GIRIGARA_SCENE_SCENE_H