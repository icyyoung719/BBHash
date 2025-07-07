#pragma once

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <winsock2.h>    // for timeval
  #include <ws2tcpip.h>    // for gettimeofday 替代
  #include <sys/timeb.h>   // for _ftime_s
  #include <chrono>
  #pragma comment(lib, "ws2_32.lib")

  inline int gettimeofday(struct timeval* tp, void* tzp)
  {
      struct _timeb timebuffer;
      _ftime_s(&timebuffer);
      tp->tv_sec = static_cast<long>(timebuffer.time);
      tp->tv_usec = static_cast<long>(timebuffer.millitm) * 1000;
      return 0;
  }

  inline void sleep_ms(int ms)
  {
      Sleep(ms);
  }

#else
  #include <sys/time.h>
  #include <unistd.h>

  inline void sleep_ms(int ms)
  {
      usleep(ms * 1000);
  }
#endif

#ifdef _WIN32
  #include <windows.h>
  using thread_t = HANDLE; // 或其他合适类型
#else
  #include <pthread.h>
  using thread_t = pthread_t;
#endif


#ifdef _MSC_VER
#include <windows.h>  // 包含 Interlocked 系列函数声明

inline void atomic_fetch_or(uint64_t* addr, uint64_t val) {
    InterlockedOr64((volatile LONG64*)addr, val);
}

inline void atomic_fetch_and(uint64_t* addr, uint64_t val) {
    InterlockedAnd64((volatile LONG64*)addr, val);
}
#else
inline void atomic_fetch_or(uint64_t* addr, uint64_t val) {
    __sync_fetch_and_or(addr, val);
}

inline void atomic_fetch_and(uint64_t* addr, uint64_t val) {
    __sync_fetch_and_and(addr, val);
}
#endif


#ifdef _MSC_VER
#include <windows.h>
inline uint64_t atomic_test_and_set(uint64_t pos, uint64_t* _bitArray)
{
    uint64_t mask = (1ULL << (pos & 63));
    uint64_t* target = _bitArray + (pos >> 6);

    // InterlockedOr64 返回的是新值，模拟原来 fetch-and-or 的返回旧值行为
    uint64_t oldval;
    do {
        oldval = *target;
    } while (InterlockedCompareExchange64((volatile LONG64*)target, oldval | mask, oldval) != oldval);

    return (oldval >> (pos & 63)) & 1;
}
#else
inline uint64_t atomic_test_and_set(uint64_t pos, uint64_t* _bitArray)
{
    uint64_t oldval = __sync_fetch_and_or(_bitArray + (pos >> 6), (1ULL << (pos & 63)));
    return (oldval >> (pos & 63)) & 1;
}
#endif
