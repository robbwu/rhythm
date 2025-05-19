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
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/scanner.cpp
        src/interpreter.cpp
        src/expr.cpp
        src/parser.cpp
        src/statement.cpp
        src/ast_printer.cpp
)

set(DEBUG_FLAGS -g -O0 -fno-omit-frame-pointer)
set(SAN_FLAGS   -fsanitize=address,undefined)

target_compile_options(${PROJECT_NAME} PRIVATE ${DEBUG_FLAGS} ${SAN_FLAGS})
target_link_options   (${PROJECT_NAME} PRIVATE ${SAN_FLAGS})


