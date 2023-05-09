#include <cstdarg>
#include <cstdio>

#include <thread>
#include <iostream>
#include <map>

#include "logger.h"

void print_prefix()
{
    static std::map<std::thread::id, size_t> thread_ids;

    std::thread::id id = std::this_thread::get_id();

    auto it = thread_ids.find(id);
    if (it == thread_ids.end())
    {
        it = thread_ids.emplace(id, thread_ids.size()).first;
    }

    std::cout << "\033[" << it->second + 31 << "m[" << it->second + 1 << "]\033[0m ";
}

void Logger::puts(const char *msg)
{
    std::lock_guard guard(mutex());
    print_prefix();
    ::puts(msg);
}

void Logger::printf(const char *format, ...)
{
    std::lock_guard guard(mutex());
    print_prefix();

    va_list argptr;
    va_start(argptr, format);

    vprintf(format, argptr);

    va_end(argptr);
}

std::mutex &Logger::mutex()
{
    static Logger logger;
    return logger.m;
}
