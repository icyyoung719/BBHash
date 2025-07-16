#pragma once

// ============================
// 平台判断
// ============================
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <sys/timeb.h>   // for _ftime_s
    #include <chrono>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/time.h>
    #include <unistd.h>
    #include <pthread.h>
#endif

#include <cstdint>

// ============================
// 类型定义
// ============================
#ifdef _WIN32
    using thread_t = HANDLE;
    typedef CRITICAL_SECTION mutex_t;
#else
    using thread_t = pthread_t;
    typedef pthread_mutex_t mutex_t;
#endif

#ifdef _MSC_VER
    using uint = unsigned int;
#endif

// ============================
// 时间相关工具
// ============================
#ifdef _WIN32

// 模拟 UNIX gettimeofday
struct timeval {
    long tv_sec;   // seconds
    long tv_usec;  // microseconds
};

inline int gettimeofday(struct timeval* tp, void* /*tzp*/) {
    struct _timeb timebuffer;
    _ftime_s(&timebuffer);
    tp->tv_sec = static_cast<long>(timebuffer.time);
    tp->tv_usec = static_cast<long>(timebuffer.millitm) * 1000;
    return 0;
}

#endif


// ============================
// 原子操作
// ============================
#ifdef _MSC_VER

inline uint64_t atomic_test_and_set(uint64_t pos, uint64_t* bitArray) {
    uint64_t mask = (1ULL << (pos & 63));
    uint64_t* target = bitArray + (pos >> 6);
    uint64_t oldval;
    do {
        oldval = *target;
    } while (InterlockedCompareExchange64(
                 reinterpret_cast<volatile LONG64*>(target),
                 oldval | mask, oldval) != oldval);
    return (oldval >> (pos & 63)) & 1;
}

#else

inline uint64_t atomic_test_and_set(uint64_t pos, uint64_t* bitArray) {
    uint64_t oldval = __sync_fetch_and_or(bitArray + (pos >> 6), (1ULL << (pos & 63)));
    return (oldval >> (pos & 63)) & 1;
}

#endif
