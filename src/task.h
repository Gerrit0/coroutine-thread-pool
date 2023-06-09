#pragma once

#include <coroutine>
#include <cassert>
#include <vector>
#include <optional>

#include "logger.h"

namespace detail
{
    template <typename TaskState>
    void ref_state(TaskState *state)
    {
        std::lock_guard lock(state->mutex);
        ++state->references;
    }

    template <typename TaskState>
    void unref_state(TaskState *state)
    {
        bool shouldDelete;
        {
            std::lock_guard lock(state->mutex);
            shouldDelete = --state->references == 0;
        }
        if (shouldDelete)
        {
            delete state;
        }
    }

    template <typename TaskState, typename Result>
    class TaskAwaiter
    {
        TaskState *m_state;

    public:
        TaskAwaiter(TaskState *state) : m_state(state)
        {
            ref_state(m_state);
        }

        ~TaskAwaiter()
        {
            unref_state(m_state);
        }

        bool await_ready() noexcept
        {
            std::lock_guard lock(m_state->mutex);
            return m_state->complete;
        }

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

        // Can't use const Result& since a reference to void is ill formed
        std::add_lvalue_reference_t<const Result> await_resume() noexcept
        {
            if constexpr (!std::is_void_v<Result>)
            {
                return *m_state->result;
            }
        }
    };
}

template <typename Result>
class Task
{
    struct TaskState
    {
        std::mutex mutex = {};
        size_t references = 1;
        bool complete = false;
        std::vector<std::coroutine_handle<>> continuations = {};
        std::optional<Result> result = std::nullopt;
    };

    // Don't use a shared_ptr here because we may manage this memory across threads
    TaskState *m_state;

public:
    Task(TaskState *state) : m_state(state)
    {
        detail::ref_state(m_state);
    }

    Task(const Task &t) : m_state(t.m_state)
    {
        detail::ref_state(m_state);
    }

    Task &operator=(const Task &t)
    {
        if (m_state != t.m_state)
        {
            detail::unref_state(m_state);
            m_state = t.m_state;
            detail::ref_state(m_state);
        }
        return *this;
    }

    ~Task()
    {
        detail::unref_state(m_state);
    }

    class promise_type
    {
        TaskState *m_state = new TaskState;

    public:
        promise_type() = default;
        promise_type(const promise_type &) = delete;
        ~promise_type()
        {
            detail::unref_state(m_state);
        }

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

    auto operator co_await() const
    {
        return detail::TaskAwaiter<TaskState, Result>(m_state);
    }
};

template <>
class Task<void>
{
    struct TaskState
    {
        std::mutex mutex = {};
        size_t references = 1;
        bool complete = false;
        std::vector<std::coroutine_handle<>> continuations = {};
    };

    // Don't use a shared_ptr here because we may manage this memory across threads
    TaskState *m_state;

public:
    Task(TaskState *state) : m_state(state)
    {
        detail::ref_state(m_state);
    }

    Task(const Task &t) : m_state(t.m_state)
    {
        detail::ref_state(m_state);
    }

    Task &operator=(const Task &t)
    {
        if (m_state != t.m_state)
        {
            detail::unref_state(m_state);
            m_state = t.m_state;
            detail::ref_state(m_state);
        }
        return *this;
    }

    ~Task()
    {
        detail::unref_state(m_state);
    }

    class promise_type
    {
        TaskState *m_state = new TaskState;

    public:
        promise_type() = default;
        promise_type(const promise_type &) = delete;
        ~promise_type()
        {
            detail::unref_state(m_state);
        }

        Task get_return_object()
        {
            return Task(m_state);
        }

        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }

        void return_void() noexcept
        {
            std::vector<std::coroutine_handle<>> continuations;

            {
                std::lock_guard lock(m_state->mutex);
                m_state->complete = true;
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

    auto operator co_await() const
    {
        return detail::TaskAwaiter<TaskState, void>(m_state);
    }
};
