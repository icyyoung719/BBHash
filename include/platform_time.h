#pragma once

// ============================
// 平台判断
// ============================
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <io.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <stdio.h>
#endif

#include <cstdint>
#include <vector>

template <typename T>
inline void write_with_file_lock(FILE* file, const std::vector<T>& buffer, size_t count)
{
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
