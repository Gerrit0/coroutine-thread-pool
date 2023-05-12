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
#include "task_utils.h"
#include "joinable_task.h"

Task<void> run_async_print(Pool &pool, int data)
{
    co_await pool.schedule();
    (void)data;
    // sleep(1);
    co_return;
}

Task<bool> other(Pool &pool)
{
    co_await pool.schedule();
    co_return true;
}

Task<void> do_work(Pool &pool)
{
    std::vector<Task<void>> tasks;
    for (auto i = 0; i < 100; i++)
    {
        tasks.emplace_back(run_async_print(pool, i));
    }
    co_await await_all(tasks);
    // for (auto &t : results)
    // {
    //     std::cout << "Got a value: " << t << "\n";
    // }

    Logger::puts("=== Do work done");

    auto [a, b] = co_await await_all(
        other(pool),
        other(pool));

    Logger::printf("Got stuff from await_all! %d, %i\n", a, b);

    co_return;
}

int main()
{
    Logger::puts("Main thread");
    Pool pool(12);
    Task work = do_work(pool);
    sync_wait(work);
    Logger::puts("Done");
}
