#include <Scene/Scene.h>
#include <UI/UIContainer.h>

Scene::Scene(CoreContext& ctx, SceneManager& manager)
    : m_ctx(ctx), m_manager(manager) {}

std::unique_ptr<UIContainer> Scene::createUI() {
    return nullptr;
}