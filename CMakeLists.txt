cmake_minimum_required(VERSION 3.10)
project(SortingFHE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenFHE targets (optional — skipped if OpenFHE not installed)
find_package(OpenFHE QUIET)
if(OpenFHE_FOUND)
    # Force include all major modules
    include_directories(/usr/local/include/openfhe)
    include_directories(/usr/local/include/openfhe/pke)
    include_directories(/usr/local/include/openfhe/binfhe)
    include_directories(/usr/local/include/openfhe/core)

    # Keep the standard variables too just in case
    include_directories(${OpenFHE_INCLUDE_DIRECTORIES})

    add_executable(sort_demo main.cpp)
    target_link_libraries(sort_demo ${OpenFHE_LIBRARIES})

    add_executable(sort8 sort8.cpp)
    target_link_libraries(sort8 ${OpenFHE_LIBRARIES})
else()
    message(STATUS "OpenFHE not found — skipping sort_demo and sort8 targets")
endif()

# SEAL-based non-batched BFV bitonic sort
find_package(SEAL 4.1 REQUIRED)
add_executable(bitonic_bfv minimal_sorts_template.cpp)
target_link_libraries(bitonic_bfv SEAL::seal)

# OpenMP parallelized targets
find_package(OpenMP REQUIRED)

add_executable(bitonic_parallel bitonic_sort.cpp)
target_link_libraries(bitonic_parallel SEAL::seal OpenMP::OpenMP_CXX)

add_executable(rank_parallel rank_sort.cpp)
target_link_libraries(rank_parallel SEAL::seal OpenMP::OpenMP_CXX)
