cmake_minimum_required(VERSION 3.26)
project(ITMO_parallel_algo_2023)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -O3")

add_executable(ITMO_parallel_algo_2023 bfs/bfs.cpp)
