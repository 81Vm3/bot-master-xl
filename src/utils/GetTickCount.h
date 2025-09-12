#pragma once

#ifdef _WIN32
    #include <windows.h>
#else
    #include <cstdint>
    uint32_t GetTickCount();

#endif

