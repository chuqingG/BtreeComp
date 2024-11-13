cmake_minimum_required(VERSION 3.8)

project(Benchmark)

set(CMAKE_CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -fdump-lang-class ")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
    # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fsanitize=address")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Wall /wd5045 /wd4464 /d1reportSingleClassLayoutNode4")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob3")
endif()

string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)
if(uppercase_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
  SET(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
  SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
endif()

# for test 
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wunused-function -Wunused-label -Wunused-value -Wunused-variable")
set(CMAKE_C_FLAGS_RELEASE     "${CMAKE_C_FLAGS_RELEASE}   -fvisibility=default")
set(CMAKE_CXX_FLAGS_RELEASE   "${CMAKE_CXX_FLAGS_RELEASE} -fvisibility=default")

### dependencies
# art
set(ART_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/third_party/art/include)
add_library(art INTERFACE)
target_include_directories(art INTERFACE ${ART_INCLUDE_DIR})




# add_library(art ${PROJECT_SOURCE_DIR}/third_party/art/include/art.hpp)
# target_include_directories(art PUBLIC ${ART_INCLUDE_DIR})
# add_library(art INTERFACE)

# target_include_directories(
#     art
#     INTERFACE
#     "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/art/include>"
#     "$<INSTALL_INTERFACE:third_party/art/include>"
# )

add_subdirectory(tree)
add_subdirectory(tree_mt)
add_subdirectory(tree_disk)

add_executable("Benchmark" "benchmark/benchmark.cpp")
add_executable("BenchmarkMT" "benchmark/benchmark_mt.cpp")
add_executable("BenchmarkDSK" "benchmark/benchmark_disk.cpp")

target_link_libraries("Benchmark" btree art)
target_link_libraries("BenchmarkMT" btree_mt)
target_link_libraries("BenchmarkDSK" btree_disk)
