cmake_minimum_required(VERSION 3.10)
project(SortingFHE)

find_package(OpenFHE REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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