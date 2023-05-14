#include <coroutine>
#include <memory>
#include <string>

#include "data_container.h"
#include "joinable_task.h"
#include "logger.h"
#include "pool.h"
#include "task_utils.h"
#include "task.h"

Task<std::shared_ptr<DataContainer>> read_data(Pool &pool, std::string file)
{
    co_await pool.schedule();
    FILE *f = fopen(file.c_str(), "r");

    if (f == nullptr)
    {
        throw std::runtime_error("File does not exist.");
    }

    auto data = std::make_shared<DataContainer>(1);
    size_t id = 0;
    double x, y, z;
    while (fscanf(f, "%lf %lf %lf", &x, &y, &z) == 3)
    {
        data->push_back({id, x, y, z});
        ++id;
    }

    Logger::printf("File contained %zu elements\n", data->size());

    fclose(f);

    co_return std::move(data);
}

Task<void> do_work(Pool &pool)
{
    auto data = co_await read_data(pool, "input.txt");

    Logger::printf("x = %f, y = %f, z = %f\n", data->at(0).x, data->at(0).y, data->at(0).z);

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
