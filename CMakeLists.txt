cmake_minimum_required(VERSION 3.20)
project(rhythm VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_GUI
       "Build the GUI (scratch) executable"
       OFF)

# Generate compile_commands.json for clangd LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/assets)

# Raylib GUI config
#find_package(raylib 3.0 REQUIRED)


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

add_executable(transpose
        src/transpose/main.cpp
        src/transpose/javascript_generator.cpp
        src/transpose/runtime.cpp
        src/transpose/transpiler.cpp
        src/scanner.cpp
        src/parser.cpp
        src/expr.cpp
        src/statement.cpp
        src/resolver.cpp
        src/interpreter.cpp
        src/ast_printer.cpp
)

find_program(NODE_EXECUTABLE NAMES node)
if(NODE_EXECUTABLE)
    file(TO_CMAKE_PATH "${NODE_EXECUTABLE}" TRANSPOSE_NODE_COMMAND)
else()
    set(TRANSPOSE_NODE_COMMAND "node")
endif()

set(TRANSPOSE_NODE_COMMAND_LITERAL "\"${TRANSPOSE_NODE_COMMAND}\"")
# target_compile_definitions(transpose PRIVATE TRANSPOSE_NODE_COMMAND=${TRANSPOSE_NODE_COMMAND_LITERAL})
target_compile_definitions(transpose
    PRIVATE
        TRANSPOSE_NODE_COMMAND="${TRANSPOSE_NODE_COMMAND}"
)

if (EMSCRIPTEN)
    set(WEB_APP_OUTPUT_DIR ${CMAKE_BINARY_DIR}/web)

    add_executable(transpose_wasm
            src/transpose/wasm_interface.cpp
            src/transpose/transpiler.cpp
            src/transpose/javascript_generator.cpp
            src/transpose/runtime.cpp
            src/scanner.cpp
            src/parser.cpp
            src/expr.cpp
            src/statement.cpp
            src/resolver.cpp
            src/interpreter.cpp
            src/ast_printer.cpp)

    set_target_properties(transpose_wasm PROPERTIES
            OUTPUT_NAME "transpose_wasm"
            RUNTIME_OUTPUT_DIRECTORY ${WEB_APP_OUTPUT_DIR})

    target_compile_options(transpose_wasm PRIVATE -fexceptions)
    target_link_options(transpose_wasm PRIVATE
            --bind
            "-sMODULARIZE=1"
            "-sEXPORT_ES6=1"
            "-sEXPORT_NAME=TransposeModule"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sASSERTIONS"
            "-sDISABLE_EXCEPTION_CATCHING=0")

    add_custom_target(rhythm_web_assets ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory ${WEB_APP_OUTPUT_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_SOURCE_DIR}/web
                    ${WEB_APP_OUTPUT_DIR}
            COMMENT "Copy Rhythm web assets")
    add_dependencies(rhythm_web_assets transpose_wasm)
endif()
include(CTest)  # defines BUILD_TESTING and enable_testing()
if(BUILD_TESTING)

    # Helper to add a test that runs a .rhy script with the beat VM
    function(add_rhythm_test name script_path)
        add_test(
            NAME    ${name}
            COMMAND $<TARGET_FILE:beat> ${script_path}
    )
        # Make it easy to locate scripts regardless of build dir
        set_tests_properties(${name} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            LABELS "examples"
            TIMEOUT 20
    )
    endfunction()

    # Helper to add a test that runs a .rhy script with the transpose backend
    function(add_transpose_test name script_path)
        add_test(
            NAME    ${name}
            COMMAND $<TARGET_FILE:transpose> ${script_path}
    )
        set_tests_properties(${name} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            LABELS "examples;transpose"
            TIMEOUT 20
    )
    endfunction()

    # Helper to add a negative test (should fail)
    function(add_rhythm_negative_test name script_path)
        add_test(
            NAME    ${name}
            COMMAND $<TARGET_FILE:beat> ${script_path}
    )
        set_tests_properties(${name} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            LABELS "negative"
            WILL_FAIL TRUE        # non-zero exit code is expected
            TIMEOUT 20
    )
    endfunction()

    # Paths to examples
    set(EX ${CMAKE_SOURCE_DIR}/examples)

    # A small, fast initial set (assert-based or self-checking)
    add_rhythm_test(examples_for                     ${EX}/for.rhy)
    add_rhythm_test(examples_array                   ${EX}/array.rhy)
    add_rhythm_test(examples_bisection               ${EX}/bisection.rhy)
    add_rhythm_test(examples_mergesort               ${EX}/mergesort.rhy)
    add_rhythm_test(examples_qsort                   ${EX}/qsort.rhy)
    add_rhythm_test(examples_sqrt_newton             ${EX}/sqrt.rhy)
    add_rhythm_test(examples_continue_while          ${EX}/continue_while.rhy)
    add_rhythm_test(examples_continue_nested         ${EX}/continue_nested.rhy)
    add_rhythm_test(examples_continue_for            ${EX}/continue_for.rhy)
    add_rhythm_test(examples_continue_hits_increment ${EX}/continue_hits_increment.rhy)
    add_rhythm_test(examples_continue_block_scope    ${EX}/continue_block_scope.rhy)
    add_rhythm_test(examples_mixed_break_continue    ${EX}/mixed_break_continue.rhy)
    add_rhythm_test(examples_avl                     ${EX}/avl.rhy)
    add_rhythm_test(examples_nqueen                  ${EX}/nqueen.rhy)

    add_test(
        NAME    transpose_emit_js_smoke
        COMMAND $<TARGET_FILE:transpose> --emit-js ${EX}/for.rhy
    )
    set_tests_properties(transpose_emit_js_smoke PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        LABELS "transpose;emit-js"
        PASS_REGULAR_EXPRESSION "const __rt ="
        TIMEOUT 20
    )

    if(NODE_EXECUTABLE AND NOT WIN32)
        add_transpose_test(transpose_for                     ${EX}/for.rhy)
        add_transpose_test(transpose_array                   ${EX}/array.rhy)
        add_transpose_test(transpose_bisection               ${EX}/bisection.rhy)
        add_transpose_test(transpose_mergesort               ${EX}/mergesort.rhy)
        add_transpose_test(transpose_qsort                   ${EX}/qsort.rhy)
        add_transpose_test(transpose_sqrt_newton             ${EX}/sqrt.rhy)
        add_transpose_test(transpose_continue_while          ${EX}/continue_while.rhy)
        add_transpose_test(transpose_continue_nested         ${EX}/continue_nested.rhy)
        add_transpose_test(transpose_continue_for            ${EX}/continue_for.rhy)
        add_transpose_test(transpose_continue_hits_increment ${EX}/continue_hits_increment.rhy)
        add_transpose_test(transpose_continue_block_scope    ${EX}/continue_block_scope.rhy)
        add_transpose_test(transpose_mixed_break_continue    ${EX}/mixed_break_continue.rhy)
        add_transpose_test(transpose_avl                     ${EX}/avl.rhy)
        add_transpose_test(transpose_nqueen                  ${EX}/nqueen.rhy)
    else()
        if(NOT NODE_EXECUTABLE)
            message(STATUS "Node.js not found; skipping transpose example tests")
        elseif(WIN32)
            message(STATUS "Skipping transpose example tests on Windows.")
        endif()
    endif()

    if(NOT WIN32)
        add_test(
        NAME    transpose_no_loop_forbidden
        COMMAND $<TARGET_FILE:transpose> --no-loop ${EX}/for.rhy
    )
        set_tests_properties(transpose_no_loop_forbidden PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        LABELS            "transpose;negative"
        WILL_FAIL         TRUE
        TIMEOUT           20
    )
    endif()


    # Optionally check for an "OK" marker when present
    foreach(t
      examples_for
      examples_array
      examples_sqrt_newton
      examples_continue_while
      examples_continue_nested
      examples_continue_for
      examples_continue_hits_increment
      examples_continue_block_scope
      examples_mixed_break_continue
  )
        set_tests_properties(${t} PROPERTIES PASS_REGULAR_EXPRESSION "OK")
    endforeach()

    if(NODE_EXECUTABLE AND NOT WIN32)
        foreach(t
      transpose_for
      transpose_array
      transpose_sqrt_newton
      transpose_continue_while
      transpose_continue_nested
      transpose_continue_for
      transpose_continue_hits_increment
      transpose_continue_block_scope
      transpose_mixed_break_continue
  )
            set_tests_properties(${t} PROPERTIES PASS_REGULAR_EXPRESSION "OK")
        endforeach()

        add_test(
            NAME    transpose_browser_runtime
            COMMAND ${NODE_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/browser_runtime_test.cjs
                    $<TARGET_FILE:transpose>
                    ${EX}/for.rhy
        )
        set_tests_properties(transpose_browser_runtime PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            LABELS "transpose;browser"
            TIMEOUT 20
        )
    endif()

    # Negative tests (parser/semantic errors expected)
    # negative cases not working yet as they crash instead of returning non-zero
    # add_rhythm_negative_test(neg_bad_double_var ${EX}/bad_double_var.rhy)
    # add_rhythm_negative_test(neg_bad_func       ${EX}/bad_func.rhy)
    # add_rhythm_negative_test(neg_bad_return     ${EX}/bad_return.rhy)

endif()


# GUI
if(BUILD_GUI)
    message(STATUS "GUI (scratch) target is enabled.")
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)
    include_directories(third_party/imgui_1.91.9b)

    add_executable(scratch
            scratch/imgui_main.cpp
            scratch/file.cpp
            scratch/embedded_fonts.cpp
            third_party/imgui_1.91.9b/imgui.cpp
            third_party/imgui_1.91.9b/imgui_draw.cpp
            third_party/imgui_1.91.9b/imgui_widgets.cpp
            third_party/imgui_1.91.9b/imgui_tables.cpp
            third_party/imgui_1.91.9b/imgui_impl_glfw.cpp
            third_party/imgui_1.91.9b/imgui_impl_opengl3.cpp
            third_party/TextEditor.cpp
            src/scanner.cpp
            src/expr.cpp
            src/parser.cpp
            src/statement.cpp
            src/ast_printer.cpp
            src/vm/chunk.cpp
            src/vm/vm.cpp
            src/vm/compiler.cpp
    )
    if (APPLE)
        target_link_libraries(scratch "-framework IOKit")
        target_link_libraries(scratch "-framework Cocoa")
        target_link_libraries(scratch "-framework OpenGL")
    endif()

    target_link_libraries(scratch glfw ${OpenGL_LIBRARIES})


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
            $<$<CONFIG:Release>:-O3 -march=native -flto -DNDEBUG>
    )

    install(TARGETS ${PROJECT_NAME} beat
            RUNTIME DESTINATION bin
            COMPONENT runtime
    )


else()
    message(STATUS "GUI (scratch) target is disabled – build with -DBUILD_GUI=ON to enable.")
endif()
