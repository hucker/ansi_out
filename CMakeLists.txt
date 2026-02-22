cmake_minimum_required(VERSION 3.14)

project(ansi_print
    VERSION     1.1.0
    LANGUAGES   C
    DESCRIPTION "Lightweight C library for colored terminal output with Rich-style markup"
)

# --- Library target -----------------------------------------------------------

add_library(ansi_print src/ansi_print.c)
target_include_directories(ansi_print PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(ansi_print PROPERTIES
    C_STANDARD          99
    C_STANDARD_REQUIRED ON
)

# Feature toggles -- pass -DANSI_PRINT_MINIMAL=ON etc. on the cmake command line
option(ANSI_PRINT_MINIMAL         "Disable all optional features"      OFF)
option(ANSI_PRINT_NO_APP_CFG      "Skip app_cfg.h auto-include"       OFF)

if(ANSI_PRINT_MINIMAL)
    target_compile_definitions(ansi_print PUBLIC ANSI_PRINT_MINIMAL)
endif()
if(ANSI_PRINT_NO_APP_CFG)
    target_compile_definitions(ansi_print PUBLIC ANSI_PRINT_NO_APP_CFG)
endif()

# --- CLI tool (optional) ------------------------------------------------------

option(ANSI_PRINT_BUILD_CLI "Build the ansiprint CLI tool" ON)

if(ANSI_PRINT_BUILD_CLI)
    add_executable(ansiprint src/main.c)
    target_link_libraries(ansiprint PRIVATE ansi_print)
endif()

# --- Tests (optional) ---------------------------------------------------------

option(ANSI_PRINT_BUILD_TESTS "Build unit tests" OFF)

if(ANSI_PRINT_BUILD_TESTS)
    enable_testing()

    add_library(unity STATIC test/unity/unity.c)
    target_include_directories(unity PUBLIC test/unity)

    add_executable(test_cprint test/test_cprint.c)
    target_link_libraries(test_cprint PRIVATE ansi_print unity)
    add_test(NAME test_cprint COMMAND test_cprint)

    # Minimal build variant
    add_executable(test_cprint_minimal test/test_cprint.c)
    target_link_libraries(test_cprint_minimal PRIVATE ansi_print unity)
    target_compile_definitions(test_cprint_minimal PRIVATE
        ANSI_PRINT_NO_APP_CFG ANSI_PRINT_MINIMAL
    )
    add_test(NAME test_cprint_minimal COMMAND test_cprint_minimal)
endif()
