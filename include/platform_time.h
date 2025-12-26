#pragma once

#include <cstdint>
#include <vector>

#ifdef _WIN32
#include "windows_sane.h"
#include <io.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <cstdio>
#endif

namespace boomphf
{

/// Thread-safe file write with platform-specific file locking
template <typename T> inline void write_with_file_lock(FILE* file, const std::vector<T>& buffer, size_t count)
{
#ifdef _WIN32
	OVERLAPPED ol = {0};
	HANDLE fileHandle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file)));
	LockFileEx(fileHandle, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ol);

	std::fwrite(buffer.data(), sizeof(T), count, file);

	UnlockFileEx(fileHandle, 0, MAXDWORD, MAXDWORD, &ol);
#else
	flockfile(file);
	std::fwrite(buffer.data(), sizeof(T), count, file);
	funlockfile(file);
#endif
}

} // namespace boomphf
