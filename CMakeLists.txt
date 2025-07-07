cmake_minimum_required(VERSION 3.10)
project(BBHash)

set(CMAKE_CXX_STANDARD 11)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# 通用编译选项
set(COMMON_FLAGS "")
if (MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
  set(COMMON_FLAGS "${COMMON_FLAGS} -lpthread")
endif()

# 特殊构建选项
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(COMMON_FLAGS "${COMMON_FLAGS} -O0 -g -DASSERTS")
elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
  set(COMMON_FLAGS "${COMMON_FLAGS} -pg")
elseif(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
  set(COMMON_FLAGS "-fsanitize=address -fno-omit-frame-pointer -O1 -fno-optimize-sibling-calls -g")
endif()

# 将选项应用到整个项目
add_compile_options(${COMMON_FLAGS})

# 包含当前目录头文件
include_directories(${CMAKE_SOURCE_DIR})

# 添加可执行文件
add_executable(Bootest bootest.cpp)
add_executable(example example.cpp)
add_executable(example_custom_hash example_custom_hash.cpp)

# 链接 pthread （非 Windows 平台）
if (NOT MSVC)
  target_link_libraries(Bootest pthread)
  target_link_libraries(example pthread)
  target_link_libraries(example_custom_hash pthread)
endif()
