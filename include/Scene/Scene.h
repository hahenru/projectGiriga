#ifndef GIRIGARA_SCENE_SCENE_H
#define GIRIGARA_SCENE_SCENE_H

#include <memory>

class CoreContext;
class SceneManager;
class UIContainer;

class Scene {
public:
    Scene(CoreContext& ctx, SceneManager& manager)
        : m_ctx(ctx), m_manager(manager) {}
    virtual ~Scene() = default;

    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void update(float dt) {}
    
    virtual void renderWorld() {}
    virtual std::unique_ptr<UIContainer> createUI();

protected:
    CoreContext& m_ctx;
    SceneManager& m_manager;
};

#endif // GIRIGARA_SCENE_SCENE_H