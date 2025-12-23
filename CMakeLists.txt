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
  if (MSVC)
    add_compile_options(/Od /Zi)
    add_definitions(-DASSERTS)
  else()
    add_compile_options(-O0 -g -DASSERTS)
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
  if (NOT MSVC)
    add_compile_options(-pg)
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
  if (NOT MSVC)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -O1 -fno-optimize-sibling-calls -g)
    add_link_options(-fsanitize=address)
  endif()
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/tests)

# Examples
add_executable(example examples/example.cpp)
add_executable(example_custom_hash examples/example_custom_hash.cpp)
add_executable(main examples/main.cpp)

# Tests
add_executable(test_endian tests/test_endian.cpp)

# Catch2-based tests
# Create a library with Catch2 main
add_library(catch_main STATIC tests/catch_main.cpp)
target_include_directories(catch_main PUBLIC ${CMAKE_SOURCE_DIR}/tests)

add_executable(test_basic tests/test_basic.cpp)
target_link_libraries(test_basic catch_main)

add_executable(test_serialization tests/test_serialization.cpp)
target_link_libraries(test_serialization catch_main)

add_executable(test_min tests/test_min.cpp)
target_link_libraries(test_min catch_main)

# Link pthread on non-Windows platforms
if (NOT MSVC)
  target_link_libraries(example pthread)
  target_link_libraries(example_custom_hash pthread)
  target_link_libraries(main pthread)
  target_link_libraries(test_endian pthread)
  target_link_libraries(test_min pthread)
  target_link_libraries(test_basic pthread)
  target_link_libraries(test_serialization pthread)
endif()

# Enable testing
enable_testing()

# Add tests to CTest
add_test(NAME test_endian COMMAND test_endian)
add_test(NAME test_basic COMMAND test_basic)
add_test(NAME test_serialization COMMAND test_serialization)

# Note: bootest.cpp has compilation issues and will be fixed in a future update
# It would be built with:
# add_executable(bootest tests/bootest.cpp)
# target_link_libraries(bootest pthread)
