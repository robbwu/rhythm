cmake_minimum_required(VERSION 3.20)
project(rhythm VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for clangd LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Generate version information
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
)

execute_process(
        COMMAND git log -1 --format=%cd --date=short
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
)

execute_process(
        COMMAND git log -1 --format=%s
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_MESSAGE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
)
# Check if repository is dirty (has uncommitted changes)
execute_process(
        COMMAND git status --porcelain
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_STATUS_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
)

# Set dirty flag based on git status output
if(GIT_STATUS_OUTPUT)
    set(GIT_DIRTY_FLAG " dirty")
else()
    set(GIT_DIRTY_FLAG "")
endif()


# Get current date and time for build date
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S UTC" UTC)

# Fallback values if git is not available
if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
endif()

if(NOT GIT_COMMIT_DATE)
    set(GIT_COMMIT_DATE "unknown")
endif()

if(NOT GIT_COMMIT_MESSAGE)
    set(GIT_COMMIT_MESSAGE "unknown")
endif()

# Generate version header
configure_file(
        ${CMAKE_SOURCE_DIR}/src/version.hpp.in
        ${CMAKE_BINARY_DIR}/generated/version.hpp
        @ONLY
)
include_directories(${CMAKE_BINARY_DIR}/generated)


# Add executable
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/scanner.cpp
        src/interpreter.cpp
        src/expr.cpp
        src/parser.cpp
        src/statement.cpp
        src/ast_printer.cpp
        src/resolver.cpp
)

add_executable( beat
        src/vm/main.cpp
        src/vm/chunk.cpp
        src/vm/vm.cpp
        src/vm/compiler.cpp
        src/scanner.cpp
        src/expr.cpp
        src/parser.cpp
        src/statement.cpp
        src/ast_printer.cpp
)

set(DEBUG_FLAGS -g -O0 -fno-omit-frame-pointer)
set(SAN_FLAGS   -fsanitize=address,undefined)

target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
        $<$<CONFIG:Debug>:${SAN_FLAGS}>
)
target_compile_options(beat PRIVATE
        $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
        $<$<CONFIG:Debug>:${SAN_FLAGS}>
)
target_link_options   (${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:${SAN_FLAGS}>
)
target_link_options   (beat PRIVATE
        $<$<CONFIG:Debug>:${SAN_FLAGS}>
)

target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-O2 -g>
)
target_compile_options(beat PRIVATE
        $<$<CONFIG:Release>:-O2 -g>
)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        COMPONENT runtime
)