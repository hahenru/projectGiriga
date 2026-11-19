#ifndef GIRIGARA_SYSTEM_THREAD_POOL_H
#define GIRIGARA_SYSTEM_THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <type_traits>
#include <cstdint>

class ThreadPool {
public:
    explicit ThreadPool(uint32_t threadCount = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> res = task->get_future();
        enqueueRaw([task]() { (*task)(); });
        return res;
    }

    void enqueue(std::move_only_function<void()> task);

    uint32_t getWorkerCount() const noexcept {
        return static_cast<uint32_t>(m_workers.size());
    }

private:
    void enqueueRaw(std::move_only_function<void()> task);
    void workerLoop();

    std::vector<std::jthread> m_workers;
    std::queue<std::move_only_function<void()>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    bool m_stopping = false;
};

#endif // GIRIGARA_SYSTEM_THREAD_POOL_H