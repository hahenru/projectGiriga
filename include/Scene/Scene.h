#ifndef GIRIGARA_SCENE_SCENE_H
#define GIRIGARA_SCENE_SCENE_H

#include <memory>
#include <string>
#include <cstdint>
#include <Core/Identifiable.h>
#include <Core/SceneClock.h>

class CoreContext;
class SceneManager;
class UIContainer;

enum class SceneExecutionMode {
    MainThread,         // 在主迴圈執行（適合依賴主執行緒全域 Lua 狀態或 UI 互動的場景）
    ThreadPoolTask,     // 丟入 ThreadPool 並行步進（適合多數副本、野外地圖）
    DedicatedThread     // 分配專屬 std::jthread 獨立固定時鐘步進（適合大型主世界地圖）
};

class Scene : public Identifiable {
public:
    Scene(CoreContext& ctx, SceneManager& manager);
    ~Scene() override = default;

    virtual void onEnter() {}
    virtual void onExit() {}

    // 新版：接收完整的局部時鐘上下文（支援全域時間倍率與局部時間群組暫停）
    virtual void update(const SceneClock& clock) {
        update(clock.sceneDt);
    }

    // 舊版相容介面（供現有 MainMenuScene / GameScene 無痛過渡）
    virtual void update(float dt) {}

    virtual void renderWorld() {}
    virtual std::unique_ptr<UIContainer> createUI();

    // 時鐘與時間暫停控制 API
    SceneClock& getClock() noexcept { return m_clock; }
    const SceneClock& getClock() const noexcept { return m_clock; }

    void setTimeScale(float scale) noexcept { m_clock.sceneTimeScale = scale; }
    float getTimeScale() const noexcept { return m_clock.sceneTimeScale; }

    void setGroupTimeScale(uint32_t groupId, float scale) {
        if (groupId == 0) return;
        m_clock.groupScales[groupId] = scale;
    }
    void clearGroupTimeScale(uint32_t groupId) {
        m_clock.groupScales.erase(groupId);
    }

    void setPaused(bool paused) noexcept { m_clock.isPaused = paused; }
    bool isPaused() const noexcept { return m_clock.isPaused; }

protected:
    CoreContext& m_ctx;
    SceneManager& m_manager;
    SceneClock m_clock;
};

#endif // GIRIGARA_SCENE_SCENE_H