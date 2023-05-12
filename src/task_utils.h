#pragma once

#include "task.h"

template <typename T>
Task<std::vector<T>> await_all(std::vector<Task<T>> &tasks)
{
    std::vector<T> result;
    result.reserve(tasks.size());

    for (auto &task : tasks)
    {
        result.emplace_back(co_await task);
    }

    co_return result;
}

Task<void> await_all(std::vector<Task<void>> &tasks)
{
    for (auto &task : tasks)
    {
        co_await task;
    }
    co_return;
}

template <typename... T>
Task<std::tuple<T...>> await_all(Task<T>... tasks)
{
    static_assert(((!std::is_void_v<T>)&&...), "tuple form of await_all cannot be used with a Task<void>");
    co_return std::make_tuple(co_await tasks...);
}
