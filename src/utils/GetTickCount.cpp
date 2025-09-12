#include "GetTickCount.h"

#ifdef _WIN32

#else
#include <chrono>

uint32_t GetTickCount()
{
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return static_cast<uint32_t>(milliseconds);
}
#endif