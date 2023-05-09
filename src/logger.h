#pragma once

#include <mutex>

struct Logger
{
    static void puts(const char *msg);

    static void printf(const char *format, ...)
        __attribute__((format(printf, 1, 2)));

private:
    static std::mutex &mutex();
    std::mutex m;
};
