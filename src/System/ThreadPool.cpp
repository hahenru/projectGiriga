#include <System/ThreadPool.h>

ThreadPool::ThreadPool(uint32_t threadCount) {
    if (threadCount == 0) {
        uint32_t hw = std::thread::hardware_concurrency();
        threadCount = (hw > 2) ? (hw - 1) : 2;
    }

    m_workers.reserve(threadCount);
    for (uint32_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back([this]() {
            workerLoop();
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_stopping = true;
    }
    m_condition.notify_all();

    // 明確等待所有 Worker 徹底退出迴圈後，才允許 m_condition 與 m_queueMutex 被銷毀
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::enqueue(std::move_only_function<void()> task) {
    if (!task) return;
    enqueueRaw(std::move(task));
}

void ThreadPool::enqueueRaw(std::move_only_function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_stopping) return;
        m_tasks.push(std::move(task));
    }
    m_condition.notify_one();
}

void ThreadPool::workerLoop() {
    while (true) {
        std::move_only_function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this]() {
                return m_stopping || !m_tasks.empty();
            });

            if (m_stopping && m_tasks.empty()) {
                return;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        if (task) {
            task();
        }
    }
}