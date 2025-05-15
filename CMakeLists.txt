cmake_minimum_required(VERSION 3.16)
project(cclox VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for clangd LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Add executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Add any libraries if needed
# target_link_libraries(${PROJECT_NAME} PRIVATE some_library)
