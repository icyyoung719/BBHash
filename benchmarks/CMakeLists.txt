cmake_minimum_required(VERSION 3.11)

add_executable(bench_bitvector bench_bitvector.cpp)
target_link_libraries(bench_bitvector PRIVATE benchmark::benchmark)

if (NOT MSVC)
  target_link_libraries(bench_bitvector PRIVATE pthread)
endif()

install(TARGETS bench_bitvector RUNTIME DESTINATION bin)
