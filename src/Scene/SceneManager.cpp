#include <Scene/SceneManager.h>
#include <algorithm>
#include <chrono>
#include <future>

SceneManager::SceneManager(CoreContext& ctx)
    : m_ctx(ctx) {}

SceneManager::~SceneManager() {
    stopAndClearAllImmediate();
}

void SceneManager::unmountScene(SceneID id) {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingOps.push_back([this, id]() {
        std::unique_ptr<SceneSlot> removedSlot;
        {
            std::lock_guard<std::mutex> scenesLock(m_scenesMutex);
            auto it = std::find_if(m_scenes.begin(), m_scenes.end(),
                                   [id](const auto& s) { return s->id == id; });
            if (it == m_scenes.end()) return;

            removedSlot = std::move(*it);
            m_scenes.erase(it);
            if (m_primarySceneId == id) {
                m_primarySceneId = m_scenes.empty() ? 0 : m_scenes.front()->id;
            }
        }

        // 先停止專屬執行緒，再安全執行 onExit()
        if (removedSlot->dedicatedWorker.joinable()) {
            removedSlot->dedicatedWorker.request_stop();
            removedSlot->dedicatedWorker.join();
        }
        if (removedSlot->scene) {
            removedSlot->scene->onExit();
        }
    });
}

void SceneManager::unmountAllScenes() {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingOps.push_back([this]() {
        stopAndClearAllImmediate();
    });
}

void SceneManager::stopAndClearAllImmediate() {
    std::vector<std::unique_ptr<SceneSlot>> oldScenes;
    {
        std::lock_guard<std::mutex> scenesLock(m_scenesMutex);
        oldScenes = std::move(m_scenes);
        m_scenes.clear();
        m_primarySceneId = 0;
    }

    // 先請求所有 DedicatedThread 停止並 join，確保不會在 onExit 時發生競態
    for (auto& slot : oldScenes) {
        if (slot && slot->dedicatedWorker.joinable()) {
            slot->dedicatedWorker.request_stop();
            slot->dedicatedWorker.join();
        }
    }

    for (auto& slot : oldScenes) {
        if (slot && slot->scene) {
            slot->scene->onExit();
        }
    }
}

void SceneManager::applyPendingOperations() {
    std::vector<std::move_only_function<void()>> ops;
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        if (m_pendingOps.empty()) return;
        ops = std::move(m_pendingOps);
        m_pendingOps.clear();
    }

    for (auto& op : ops) {
        if (op) op();
    }
}

void SceneManager::stepSlot(SceneSlot& slot, float rawDt) {
    if (!slot.active || !slot.scene) return;

    if (slot.fixedTickRate <= 0) {
        // 變動步長模式（跟隨傳入的幀時間）
        slot.scene->getClock().advance(rawDt);
        slot.scene->update(slot.scene->getClock());
    } else {
        // 固定步長累加器模式（確保不同執行緒速率下物理與技能判定一致）
        const float fixedStep = 1.0f / static_cast<float>(slot.fixedTickRate);
        // 防止單幀卡頓引發螺旋死亡（最高單幀補償 0.25 秒）
        slot.accumulator += std::min(rawDt, 0.25f);

        while (slot.accumulator >= fixedStep) {
            slot.scene->getClock().advance(fixedStep);
            slot.scene->update(slot.scene->getClock());
            slot.accumulator -= fixedStep;
        }
    }
}

void SceneManager::startDedicatedLoop(SceneSlot& slot) {
    SceneSlot* rawSlot = &slot;
    slot.dedicatedWorker = std::jthread([this, rawSlot](std::stop_token st) {
        const int rate = (rawSlot->fixedTickRate > 0) ? rawSlot->fixedTickRate : 60;
        const auto tickDuration = std::chrono::duration<double>(1.0 / static_cast<double>(rate));
        auto lastTime = std::chrono::steady_clock::now();

        while (!st.stop_requested()) {
            auto now = std::chrono::steady_clock::now();
            float rawDt = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;

            stepSlot(*rawSlot, rawDt);

            auto elapsed = std::chrono::steady_clock::now() - now;
            if (elapsed < tickDuration) {
                std::this_thread::sleep_for(tickDuration - elapsed);
            }
        }
    });
}

void SceneManager::update(float rawDt) {
    // 1. 先在安全點套用掛載/卸載/切換操作
    applyPendingOperations();

    // 2. 收集本幀需要更新的 MainThread 與 ThreadPoolTask 場景
    std::vector<SceneSlot*> mainThreadSlots;
    std::vector<SceneSlot*> poolSlots;

    {
        std::lock_guard<std::mutex> scenesLock(m_scenesMutex);
        for (auto& slot : m_scenes) {
            if (!slot || !slot->active) continue;
            if (slot->execMode == SceneExecutionMode::MainThread) {
                mainThreadSlots.push_back(slot.get());
            } else if (slot->execMode == SceneExecutionMode::ThreadPoolTask) {
                poolSlots.push_back(slot.get());
            }
            // DedicatedThread 場景由各自獨立執行緒自主前進，不在此干預
        }
    }

    // 3. 將 ThreadPoolTask 場景派發給執行緒池並行運算
    std::vector<std::future<void>> futures;
    if (!poolSlots.empty() && m_ctx.threadPool) {
        futures.reserve(poolSlots.size());
        for (SceneSlot* slot : poolSlots) {
            futures.push_back(m_ctx.threadPool->submit([this, slot, rawDt]() {
                stepSlot(*slot, rawDt);
            }));
        }
    } else {
        for (SceneSlot* slot : poolSlots) {
            stepSlot(*slot, rawDt);
        }
    }

    // 4. 同時在主執行緒推進 MainThread 場景（如 LuaScene / 主選單）
    for (SceneSlot* slot : mainThreadSlots) {
        stepSlot(*slot, rawDt);
    }

    // 5. 等待本幀所有 ThreadPool 場景步進完成
    for (auto& fut : futures) {
        fut.get();
    }
}

Scene* SceneManager::getScene(SceneID id) const {
    std::lock_guard<std::mutex> scenesLock(m_scenesMutex);
    for (const auto& slot : m_scenes) {
        if (slot && slot->id == id) return slot->scene.get();
    }
    return nullptr;
}

Scene* SceneManager::findSceneByTag(const std::string& tag) const {
    std::lock_guard<std::mutex> scenesLock(m_scenesMutex);
    for (const auto& slot : m_scenes) {
        if (slot && slot->tag == tag) return slot->scene.get();
    }
    return nullptr;
}

std::vector<SceneID> SceneManager::getMountedSceneIDs() const {
    std::lock_guard<std::mutex> scenesLock(m_scenesMutex);
    std::vector<SceneID> ids;
    ids.reserve(m_scenes.size());
    for (const auto& slot : m_scenes) {
        if (slot) ids.push_back(slot->id);
    }
    return ids;
}

Scene* SceneManager::getCurrentScene() const {
    std::lock_guard<std::mutex> scenesLock(m_scenesMutex);
    if (m_primarySceneId != 0) {
        for (const auto& slot : m_scenes) {
            if (slot && slot->id == m_primarySceneId) return slot->scene.get();
        }
    }
    return m_scenes.empty() ? nullptr : m_scenes.front()->scene.get();
}