# Cross-Platform Compatibility

This document describes the cross-platform compatibility features of BBHash, ensuring the library works correctly on both Windows and Linux systems (x86 and x64 architectures).

## Overview

BBHash is designed to work seamlessly across different platforms:
- **Operating Systems**: Windows (MSVC), Linux (GCC/Clang)
- **Architectures**: x86 (32-bit), x64 (64-bit)
- **Endianness**: Little-endian and big-endian systems

## Cross-Platform Features

### 1. Time Measurement

**Challenge**: Platform-specific time functions like `gettimeofday()` (POSIX) and `GetSystemTime()` (Windows) are not portable.

**Solution**: All time measurements now use C++11 `<chrono>` library, which is fully cross-platform:

```cpp
// Cross-platform timing
auto t_begin = std::chrono::high_resolution_clock::now();
// ... operations ...
auto t_end = std::chrono::high_resolution_clock::now();
double elapsed = std::chrono::duration<double>(t_end - t_begin).count();
```

**Files affected**:
- `examples/example.cpp`
- `examples/example_custom_hash.cpp`
- `tests/bootest.cpp`

### 2. File Locking

**Challenge**: File locking mechanisms differ between Windows and POSIX systems.

**Solution**: Platform-specific file locking is abstracted in `include/platform_time.h`:

```cpp
#ifdef _WIN32
  // Windows: Use LockFileEx/UnlockFileEx
  OVERLAPPED ol = {0};
  HANDLE fileHandle = (HANDLE)_get_osfhandle(_fileno(file));
  LockFileEx(fileHandle, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ol);
  fwrite(buffer.data(), sizeof(T), count, file);
  UnlockFileEx(fileHandle, 0, MAXDWORD, MAXDWORD, &ol);
#else
  // POSIX: Use flockfile/funlockfile
  flockfile(file);
  fwrite(buffer.data(), sizeof(T), count, file);
  funlockfile(file);
#endif
```

### 3. Endianness-Safe Serialization

**Challenge**: Binary data saved on one system may not load correctly on systems with different byte order.

**Solution**: All serialization uses little-endian format. See [ENDIANNESS.md](ENDIANNESS.md) for details.

- Cross-architecture file compatibility
- `.mphf` files are portable between systems
- No performance impact on little-endian systems (x86/x64)

### 4. Threading

**Challenge**: Windows doesn't support POSIX threads (pthread).

**Solution**: 
- Uses C++11 standard `<thread>` library for threading (cross-platform)
- CMake configuration conditionally links pthread only on non-Windows platforms:

```cmake
# Link pthread on non-Windows platforms
if (NOT MSVC)
  target_link_libraries(example pthread)
  # ... other targets
endif()
```

### 5. Build System

**Challenge**: Compiler-specific flags differ between MSVC, GCC, and Clang.

**Solution**: CMake configuration handles platform-specific build options:

```cmake
# Disable warnings
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/w>
    $<$<CXX_COMPILER_ID:GNU>:-w>
    $<$<CXX_COMPILER_ID:Clang>:-w>
)

# Debug build options
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  if (MSVC)
    add_compile_options(/Od /Zi)
    add_definitions(-DASSERTS)
  else()
    add_compile_options(-O0 -g -DASSERTS)
  endif()
endif()
```

## Building on Different Platforms

### Linux (GCC/Clang)

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Windows (MSVC)

```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Cross-Compilation

The library is header-only for the core functionality, making it easy to cross-compile. All platform-specific code is isolated and properly guarded with preprocessor directives.

## Platform Detection

The codebase uses standard platform detection macros:

- **Windows**: `_WIN32` (defined on both 32-bit and 64-bit Windows)
- **MSVC Compiler**: `MSVC` (in CMake) or `_MSC_VER` (in C++)
- **GCC/Clang**: Detected via CMake `CXX_COMPILER_ID`

## Testing Cross-Platform Compatibility

Run the test suite on each platform:

```bash
./test_endian    # Tests endianness-safe serialization
./example        # Basic functionality test
./main          # Advanced usage test
```

For comprehensive testing:
1. Build `.mphf` file on one platform
2. Copy to another platform (e.g., Linux → Windows)
3. Load and verify on the second platform

## Best Practices for Maintainers

When adding new code:

1. **Avoid platform-specific headers**: Use STL cross-platform alternatives
   - ❌ `<sys/time.h>`, `<unistd.h>`, `<windows.h>`
   - ✅ `<chrono>`, `<thread>`, `<mutex>`

2. **Use proper guards for platform-specific code**:
   ```cpp
   #ifdef _WIN32
     // Windows-specific code
   #else
     // POSIX-specific code
   #endif
   ```

3. **Test on multiple platforms**: Ensure changes compile and work on both Windows and Linux

4. **Use CMake generator expressions** for compiler-specific flags:
   ```cmake
   add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W4>)
   ```

## Known Limitations

- **Profile build mode**: Only available on GCC/Clang (uses `-pg` flag)
- **Sanitize build mode**: Only available on GCC/Clang (uses `-fsanitize=address`)
- **bootest.cpp**: Currently has compilation issues (noted in CMakeLists.txt)

## Dependencies

### Required
- CMake 3.10 or higher
- C++17 compiler (MSVC 2017+, GCC 7+, Clang 5+)

### Platform-Specific
- **Linux/Unix**: pthread (automatically linked)
- **Windows**: ws2_32.lib (for Windows sockets, if needed)

## Summary

The BBHash library is fully cross-platform and portable:
- ✅ Compiles on Windows (MSVC) and Linux (GCC/Clang)
- ✅ Works on x86 and x64 architectures
- ✅ Binary files (`.mphf`) are portable between platforms
- ✅ Uses only standard C++ features where possible
- ✅ Platform-specific code is properly isolated and documented

For issues or questions about cross-platform compatibility, please refer to the examples in this repository or open an issue on GitHub.
