cmake_minimum_required(VERSION 3.10)
project(SortingFHE)

# Find OpenFHE - this works because you ran 'sudo make install'
find_package(OpenFHE REQUIRED)

# Use C++17 (required by OpenFHE)
set(CMAKE_CXX_STANDARD 17)

add_executable(sort_demo main.cpp)

# Link against the OpenFHE libraries
target_link_libraries(sort_demo OpenFHE::openfhe)