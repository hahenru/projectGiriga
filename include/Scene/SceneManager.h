#ifndef GIRIGARA_SCENE_SCENE_MANAGER_H
#define GIRIGARA_SCENE_SCENE_MANAGER_H

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <cstdint>

#include <Core/Identifiable.h>
#include <Core/CoreContext.h>
#include <Scene/Scene.h>
#include <Event/EventBus.h>

using SceneID = uint64_t;

struct SceneSlot {
    SceneID id = 0;
    std::string tag;
    std::unique_ptr<Scene> scene;
    SceneExecutionMode execMode = SceneExecutionMode::MainThread;
    int fixedTickRate = 60;         // 該場景獨立的邏輯更新頻率 (Hz)，0 表示跟隨傳入 rawDt
    float accumulator = 0.0f;       // 獨立時間累加器
    bool active = true;

    // 若 execMode == DedicatedThread，則由專屬背景執行緒獨立推進時鐘
    std::jthread dedicatedWorker;
};

class SceneManager : public Identifiable {
public:
    explicit SceneManager(CoreContext& ctx);
    ~SceneManager() override;

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    // 推進所有非 DedicatedThread 的場景（MainThread 與 ThreadPoolTask 並行計算）
    void update(float rawDt);
    void applyPendingOperations();

    // ★ 多場景並行掛載 API（MMORPG 多地圖 / 客戶端多視角同構核心）
    template<typename T, typename... Args>
    SceneID mountScene(const std::string& tag,
                       SceneExecutionMode mode = SceneExecutionMode::ThreadPoolTask,
                       int fixedTickRate = 60,
                       Args&&... args) {
        auto scenePtr = std::make_unique<T>(m_ctx, *this, std::forward<Args>(args)...);
        if (!tag.empty()) {
            scenePtr->setName(tag);
        }
        SceneID newId = scenePtr->getID().get();

        std::lock_guard<std::mutex> lock(m_pendingMutex);
        m_pendingOps.push_back([this, newId, tag, mode, fixedTickRate, s = std::move(scenePtr)]() mutable {
            auto slot = std::make_unique<SceneSlot>();
            slot->id = newId;
            slot->tag = std::move(tag);
            slot->scene = std::move(s);
            slot->execMode = mode;
            slot->fixedTickRate = fixedTickRate;

            slot->scene->onEnter();

            if (mode == SceneExecutionMode::DedicatedThread) {
                startDedicatedLoop(*slot);
            }

            std::lock_guard<std::mutex> scenesLock(m_scenesMutex);
            m_scenes.push_back(std::move(slot));
        });

        return newId;
    }

    void unmountScene(SceneID id);
    void unmountAllScenes();

    // 查詢特定場景（供多視口 Viewport 渲染或 GM 監控綁定）
    Scene* getScene(SceneID id) const;
    Scene* findSceneByTag(const std::string& tag) const;
    std::vector<SceneID> getMountedSceneIDs() const;

    // ★ 向下相容單一主場景切換（自動清空舊場景並設為 primaryScene）
    template<typename T, typename... Args>
    void switchScene(Args&&... args) {
        auto scenePtr = std::make_unique<T>(m_ctx, *this, std::forward<Args>(args)...);
        SceneID newId = scenePtr->getID().get();

        std::lock_guard<std::mutex> lock(m_pendingMutex);
        m_pendingOps.push_back([this, newId, s = std::move(scenePtr)]() mutable {
            stopAndClearAllImmediate();

            auto slot = std::make_unique<SceneSlot>();
            slot->id = newId;
            slot->tag = "PrimaryScene";
            slot->scene = std::move(s);
            slot->execMode = SceneExecutionMode::MainThread;
            slot->fixedTickRate = 0; // 跟隨主迴圈 dt

            slot->scene->onEnter();

            // 移交新場景的 UI 樹所有權（僅在有 UI 時發布，避免空指針蓋掉預設容器）
            if (m_ctx.eventBus) {
                if (auto ui = slot->scene->createUI()) {
                    m_ctx.eventBus->publish(SceneUIChangedEvent{std::move(ui)});
                }
            }

            std::lock_guard<std::mutex> scenesLock(m_scenesMutex);
            m_primarySceneId = newId;
            m_scenes.push_back(std::move(slot));
        });
    }

    Scene* getCurrentScene() const;

private:
    void stepSlot(SceneSlot& slot, float rawDt);
    void startDedicatedLoop(SceneSlot& slot);
    void stopAndClearAllImmediate();

    CoreContext& m_ctx;

    mutable std::mutex m_scenesMutex;
    std::vector<std::unique_ptr<SceneSlot>> m_scenes;
    SceneID m_primarySceneId = 0;

    std::mutex m_pendingMutex;
    std::vector<std::move_only_function<void()>> m_pendingOps;
};

#endif // GIRIGARA_SCENE_SCENE_MANAGER_H