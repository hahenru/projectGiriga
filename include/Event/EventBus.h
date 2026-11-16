#ifndef GIRIGARA_EVENT_EVENT_BUS_H
#define GIRIGARA_EVENT_EVENT_BUS_H

#include <Event/Event.h>
#include <Core/Depersonalization.h>
#include <unordered_map>
#include <vector>
#include <functional>
#include <typeindex>
#include <utility>
#include <algorithm>
#include <mutex>
#include <queue>

class EventBus {
public:
    template <typename T>
    using EventCallback = std::function<void(T&)>;

    // 1. 一般訂閱（回傳專屬訂閱憑證 Depersonalization，可用於單筆退訂）
    template <typename T>
    Depersonalization subscribe(EventCallback<T> callback, Depersonalization ownerId = Depersonalization(0)) {
        auto type = std::type_index(typeid(T));
        Depersonalization subId; // 自動生成唯一 ID
        m_subscribers[type].push_back({
            subId,
            ownerId,
            [cb = std::move(callback)](Event& event) { cb(static_cast<T&>(event)); },
                                      true
        });
        return subId;
    }

    // 2. 優先訂閱（排在佇列最前端）
    template <typename T>
    Depersonalization subscribePriority(EventCallback<T> callback, Depersonalization ownerId = Depersonalization(0)) {
        auto type = std::type_index(typeid(T));
        Depersonalization subId;
        m_subscribers[type].insert(m_subscribers[type].begin(), {
            subId,
            ownerId,
            [cb = std::move(callback)](Event& event) { cb(static_cast<T&>(event)); },
                                   true
        });
        return subId;
    }

    // 3. 依單筆訂閱憑證退訂
    template <typename T>
    void unsubscribe(Depersonalization subId) {
        if (!subId.isValid()) return;
        unsubscribe(std::type_index(typeid(T)), subId);
    }

    void unsubscribe(std::type_index type, Depersonalization subId) {
        if (!subId.isValid()) return;
        auto it = m_subscribers.find(type);
        if (it == m_subscribers.end()) return;

        for (auto& entry : it->second) {
            if (entry.subId == subId && entry.active) {
                entry.active = false;
                m_hasDirtyEntries = true;
                break;
            }
        }
        if (m_dispatchDepth == 0) cleanupInactive();
    }

    // 4. ★ 依物件擁有者 ID 一鍵退訂該物件註冊的所有事件（適合在模組/視窗解構時呼叫）
    void unsubscribeByOwner(Depersonalization ownerId) {
        if (!ownerId.isValid()) return;
        for (auto& [type, list] : m_subscribers) {
            for (auto& entry : list) {
                if (entry.ownerId == ownerId && entry.active) {
                    entry.active = false;
                    m_hasDirtyEntries = true;
                }
            }
        }
        if (m_dispatchDepth == 0) cleanupInactive();
    }
	// 5. ★ 執行緒安全：允許背景執行緒排入事件，延遲至主執行緒呼叫 dispatchQueued() 時派發
    template <typename T>
    void enqueue(T&& event) {
        using EventType = std::decay_t<T>;
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_eventQueue.push([this, ev = std::forward<T>(event)]() mutable {
            publish(ev);
        });
    }

    // 必須由主執行緒在主迴圈中呼叫，清空並派發所有背景執行緒排入的事件
    void dispatchQueued() {
        std::queue<std::function<void()>> localQueue;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (m_eventQueue.empty()) return;
            localQueue.swap(m_eventQueue);
        }
        while (!localQueue.empty()) {
            localQueue.front()();
            localQueue.pop();
        }
    }

    // 發布左值事件
    template <typename T>
    void publish(T& event) {
        auto type = std::type_index(typeid(T));
        auto it = m_subscribers.find(type);
        if (it == m_subscribers.end()) return;

        ++m_dispatchDepth;
        auto& list = it->second;
        for (size_t i = 0; i < list.size(); ++i) {
            if (!list[i].active) continue;
            list[i].handler(event);
            if (event.isHandled) break;
        }
        --m_dispatchDepth;

        if (m_dispatchDepth == 0 && m_hasDirtyEntries) {
            cleanupInactive();
        }
    }

    // 發布右值暫時物件
    template <typename T>
    void publish(T&& event) {
        T ev = std::move(event);
        publish(ev);
    }

private:
    struct SubscriberEntry {
        Depersonalization subId;
        Depersonalization ownerId;
        std::function<void(Event&)> handler;
        bool active = true;
    };

    void cleanupInactive() {
        for (auto& [type, list] : m_subscribers) {
            std::erase_if(list, [](const SubscriberEntry& e) { return !e.active; });
        }
        m_hasDirtyEntries = false;
    }

    std::unordered_map<std::type_index, std::vector<SubscriberEntry>> m_subscribers;
    int m_dispatchDepth = 0;
    bool m_hasDirtyEntries = false;
	std::mutex m_queueMutex;
    std::queue<std::function<void()>> m_eventQueue;
};

#endif // GIRIGARA_EVENT_EVENT_BUS_H
