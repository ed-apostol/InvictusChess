cmake_minimum_required(VERSION 3.9)
project(invictus CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

set (CMAKE_CXX_FLAGS "-Wall -O3 -msse3 -mpopcnt -mbmi2 -DNDEBUG")
set (CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")
set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
set (THREADS_PREFER_PTHREAD_FLAG TRUE)
set (CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
find_package(Threads REQUIRED)

file(GLOB SRCS src/*.cpp src/*.h)
add_executable(invictus ${SRCS})
target_link_libraries(invictus Threads::Threads)
