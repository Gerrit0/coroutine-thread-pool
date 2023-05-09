#pragma once

#include <condition_variable>
#include <coroutine>
#include <cstdint>
#include <list>
#include <mutex>
#include <queue>
#include <thread>

#include "logger.h"

class Pool
{
public:
    explicit Pool(const std::size_t threadCount)
    {
        for (std::size_t i = 0; i < threadCount; ++i)
        {
            std::thread worker_thread([this]()
                                      { this->thread_loop(); });
            m_threads.push_back(std::move(worker_thread));
        }
    }

    ~Pool()
    {
        Logger::puts("~Pool");
        shutdown();
    }

    auto schedule()
    {
        struct awaiter
        {
            Pool *m_pool;

            constexpr bool await_ready() const noexcept { return false; }
            constexpr void await_resume() const noexcept {}
            void await_suspend(std::coroutine_handle<> coro) const noexcept
            {
                Logger::printf("Scheduling %p to run on another thread\n", coro.address());
                m_pool->enqueue_task(coro);
            }
        };
        return awaiter{this};
    }

private:
    std::list<std::thread> m_threads;

    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::queue<std::coroutine_handle<>> m_coros;

    bool m_running = true;

    void thread_loop()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait(lock, [&]()
                        { return !m_running || !m_coros.empty(); });

            if (!m_running)
            {
                break;
            }

            auto coro = m_coros.front();
            m_coros.pop();
            lock.unlock();
            coro.resume();
        }
    }

    void enqueue_task(std::coroutine_handle<> coro) noexcept
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_coros.emplace(coro);
        m_cond.notify_one();
    }

    void shutdown()
    {
        m_running = false;
        m_cond.notify_all();
        while (m_threads.size() > 0)
        {
            std::thread &thread = m_threads.back();
            if (thread.joinable())
            {
                thread.join();
            }
            m_threads.pop_back();
        }
    }
};
