#include <string>
#include <queue>
#include <iostream>
#include <thread>
#include <vector>
#include <optional>

#include <coroutine>
#include <condition_variable>

#include "pool.h"
#include "logger.h"
#include "task.h"
#include "joinable_task.h"

Task<int> run_async_print(Pool &pool, int data)
{
    co_await pool.schedule();
    sleep(1);
    co_return data * 5;
}

Task<int> do_work(Pool &pool)
{
    std::vector<Task<int>> tasks;
    for (auto i = 0; i < 10; i++)
    {
        tasks.emplace_back(run_async_print(pool, i));
    }
    for (auto &t : tasks)
    {
        auto &value = co_await t;
        std::cout << "Got a value: " << value << "\n";
    }

    Logger::puts("=== Do work done");
    co_return 123;
}

int main()
{
    Logger::puts("Main thread");
    Pool pool(12);
    Task work = do_work(pool);
    sync_wait(work);
    Logger::puts("Done");
}
