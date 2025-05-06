# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Name of project
project(imu_uart_parser)

# Use C++17 standard
set(CMAKE_CXX_STANDARD 17)

# Set the output directory for the compiled executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add the include/ folder so headers can be found
include_directories(include)

# Grab all .cpp files inside the src/ folder
file(GLOB SOURCES
    src/*.cpp
)

# Define the executable target and link all source files
add_executable(imu_uart_parser ${SOURCES})
