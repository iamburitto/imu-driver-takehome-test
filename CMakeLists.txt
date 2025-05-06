include(FetchContent)

# Ensures compatibility—won’t run on too-old versions
cmake_minimum_required(VERSION 3.10)

# Name of project internally in CMake
project(imu_uart_parser)

# Use C++17 standard
set(CMAKE_CXX_STANDARD 17)

# Set the output directory for binaries
# i.e. puts the imu_uart_parser binary into build/bin/ instead of cluttering top-level
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Tell the compiler to search for headers in include/
include_directories(include)

# 👇 Include FetchContent to allow test/CMakeLists.txt to fetch Catch2
include(FetchContent)

# Collect all .cpp files in src/ so you don’t have to list them one by one
file(GLOB SOURCES
    src/*.cpp
)

# Define the final executable target and link all .cpp source files into the executable
add_executable(imu_uart_parser ${SOURCES})

# 👇 Enable testing system and add test subdirectory
enable_testing()
add_subdirectory(test)