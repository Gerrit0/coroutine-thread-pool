#pragma once

#include <atomic>

#include "task.h"

struct JoinableTask
{
    struct promise_type
    {
        std::atomic_flag *m_flag = nullptr;

        JoinableTask get_return_object() noexcept
        {
            return JoinableTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        // Critically important for this to always suspend. If we didn't,
        // then we would try to run before m_event was set and blow up.
        std::suspend_always initial_suspend() const noexcept { return {}; }

        auto final_suspend() noexcept
        {
            struct awaiter
            {
                bool await_ready() const noexcept { return false; }

                void await_suspend(std::coroutine_handle<promise_type> continuation) const noexcept
                {
                    std::atomic_flag *event = continuation.promise().m_flag;
                    event->test_and_set();
                    event->notify_all();
                }

                void await_resume() noexcept {};
            };

            return awaiter{};
        }

        void unhandled_exception() noexcept
        {
            Logger::puts("Unhandled exception while synchonously waiting for task to finish");
            exit(1);
        }
    };

    JoinableTask(std::coroutine_handle<promise_type> coro)
        : m_handle(coro)
    {
    }

    ~JoinableTask()
    {
        if (m_handle)
        {
            m_handle.destroy();
        }
    }

    void run()
    {
        std::atomic_flag flag;

        m_handle.promise().m_flag = &flag;
        m_handle.resume();
        flag.wait(false);

        m_handle.destroy();
        m_handle = nullptr;
    }

private:
    std::coroutine_handle<promise_type> m_handle = nullptr;
};

template <typename Result>
Result sync_wait(Task<Result> &task)
{
    static auto joiner = [](Task<Result> &t, std::optional<Result> &result) -> JoinableTask
    {
        result = co_await t;
    };

    std::optional<Result> result;
    auto wait_task = joiner(task, /* out */ result);
    wait_task.run();
    return *result;
}
