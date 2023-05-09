#pragma once

#include <coroutine>
#include <cassert>
#include <memory>
#include <vector>
#include <optional>

#include "logger.h"

template <typename Result>
    requires(!std::is_void_v<Result>)
class Task
{
    struct TaskState
    {
        std::mutex mutex = {};
        bool complete = false;
        std::vector<std::coroutine_handle<>> continuations = {};
        std::optional<Result> result = std::nullopt;
    };

    std::shared_ptr<TaskState> m_state;

public:
    Task(std::shared_ptr<TaskState> state) : m_state(state)
    {
    }

    Task(const Task &t) = default;
    Task &operator=(const Task &) = default;
    Task(Task &&t) = default;
    Task &operator=(Task &&t) = default;

    ~Task() = default;

    class promise_type
    {
        std::shared_ptr<TaskState> m_state = std::make_shared<TaskState>();

    public:
        promise_type() = default;
        promise_type(const promise_type &) = delete;
        ~promise_type() = default;

        Task get_return_object()
        {
            return Task(m_state);
        }

        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }

        void return_value(const Result &value) noexcept(std::is_nothrow_copy_constructible_v<Result>)
        {
            std::vector<std::coroutine_handle<>> continuations;

            {
                std::lock_guard lock(m_state->mutex);
                m_state->complete = true;
                m_state->result = value;
                std::swap(continuations, m_state->continuations);
            }

            for (const auto &co : continuations)
            {
                co.resume();
            }
        }

        void return_value(Result &&value) noexcept(std::is_nothrow_move_constructible_v<Result>)
        {
            std::vector<std::coroutine_handle<>> continuations;

            {
                std::lock_guard lock(m_state->mutex);
                m_state->complete = true;
                m_state->result = std::move(value);
                std::swap(continuations, m_state->continuations);
            }

            for (const auto &co : continuations)
            {
                co.resume();
            }
        }

        void unhandled_exception() noexcept
        {
            Logger::puts("Unhandled exception in coroutine");
            exit(1);
        }
    };

    auto operator co_await()
    {
        struct task_awaiter
        {
            std::shared_ptr<TaskState> m_state;

            bool await_ready() noexcept
            {
                std::lock_guard lock(m_state->mutex);
                return m_state->complete;
            }

            // Since we return void, the current coroutine will always be suspended.
            bool await_suspend(std::coroutine_handle<> continuation) noexcept
            {
                // Need to schedule continuation to be called when this task is ready
                std::lock_guard lock(m_state->mutex);
                if (m_state->complete)
                {
                    // await_ready returned false, but then another thread completed the promise
                    // and now we can continue without scheduling this.
                    return false;
                }
                else
                {
                    m_state->continuations.emplace_back(continuation);
                    return true;
                }
            }

            const Result &await_resume() noexcept
            {
                return *m_state->result;
            }
        };

        return task_awaiter{m_state};
    }
};
