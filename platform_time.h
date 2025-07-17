#pragma once

// ============================
// 平台判断
// ============================
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <pthread.h>
#endif

#include <cstdint>


template<typename T>
inline void write_with_file_lock(FILE* file, const std::vector<T>& buffer, size_t count) {
#ifdef _WIN32
    OVERLAPPED ol = {0};
    HANDLE fileHandle = (HANDLE)_get_osfhandle(_fileno(file));
    LockFileEx(fileHandle, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ol);

    fwrite(buffer.data(), sizeof(T), count, file);

    UnlockFileEx(fileHandle, 0, MAXDWORD, MAXDWORD, &ol);
#else
    flockfile(file);
    fwrite(buffer.data(), sizeof(T), count, file);
    funlockfile(file);
#endif
}


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
