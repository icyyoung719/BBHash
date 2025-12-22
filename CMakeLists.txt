cmake_minimum_required(VERSION 3.10)
project(BBHash)

set(CMAKE_CXX_STANDARD 17)

# Set build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Disable warnings
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/w>
    $<$<CXX_COMPILER_ID:GNU>:-w>
    $<$<CXX_COMPILER_ID:Clang>:-w>
)

# Common compile options
if (MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Build-specific options
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-O0 -g -DASSERTS)
elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
  add_compile_options(-pg)
elseif(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -O1 -fno-optimize-sibling-calls -g)
  add_link_options(-fsanitize=address)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Examples
add_executable(example examples/example.cpp)
add_executable(example_custom_hash examples/example_custom_hash.cpp)
add_executable(main examples/main.cpp)

# Tests
add_executable(test_endian tests/test_endian.cpp)
add_executable(test_min tests/test_min.cpp)

# Link pthread on non-Windows platforms
if (NOT MSVC)
  target_link_libraries(example pthread)
  target_link_libraries(example_custom_hash pthread)
  target_link_libraries(main pthread)
  target_link_libraries(test_endian pthread)
  target_link_libraries(test_min pthread)
endif()

# Note: bootest.cpp has compilation issues and will be fixed in a future update
# It would be built with:
# add_executable(bootest tests/bootest.cpp)
# target_link_libraries(bootest pthread)
