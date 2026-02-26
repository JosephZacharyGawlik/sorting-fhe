cmake_minimum_required(VERSION 3.10)
project(SortingFHE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SEAL 4.1 REQUIRED)
find_package(OpenMP REQUIRED)

add_executable(rank_parallel rank_sort.cpp)
target_link_libraries(rank_parallel SEAL::seal OpenMP::OpenMP_CXX)

add_executable(bitonic_parallel bitonic_sort.cpp)
target_link_libraries(bitonic_parallel SEAL::seal OpenMP::OpenMP_CXX)

add_executable(rank_32k rank_sort_32k.cpp)
target_link_libraries(rank_32k SEAL::seal OpenMP::OpenMP_CXX)
