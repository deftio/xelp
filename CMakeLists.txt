# =============================================================================
# xelp -- Embedded CLI / Script Interpreter Library
# =============================================================================
#
# This file serves two roles:
#
#   1. ESP-IDF component  -- when built inside the ESP-IDF framework, the
#      idf_component_register() call below is all that's needed.
#
#   2. Plain CMake library -- for any other CMake-based project:
#
#        add_subdirectory(path/to/xelp)
#        target_link_libraries(my_app PRIVATE xelp)
#
#      That gives you the include path (src/) and the compiled library.
#
# ---- Build-time configuration ----
#
# xelp is configured entirely through C preprocessor defines in
# src/xelpcfg.h.  Features (CLI mode, KEY mode, THR mode, line editing,
# built-in help, etc.) are toggled by commenting / uncommenting #defines
# in that file.  There are NO CMake options that mirror those flags --
# edit xelpcfg.h directly.
#
# If you need a project-specific configuration WITHOUT modifying xelpcfg.h,
# define XELP_CONFIG_OVERRIDE before xelp.h is included.  This causes
# xelpcfg.h to #include "xelp_ovr.h" instead of using its built-in
# defaults.  Place your xelp_ovr.h on the include path and set all the
# XELP_ENABLE_* / XELPKEY_* defines there.
#
# Example with an override file:
#
#   target_compile_definitions(xelp PUBLIC XELP_CONFIG_OVERRIDE)
#   target_include_directories(xelp PUBLIC ${CMAKE_SOURCE_DIR}/my_config)
#   # ^ my_config/ contains your custom xelp_ovr.h
#
# ---- Register type ----
#
# xelp's internal registers (mR[]) default to plain `int`.  To change
# the type (e.g. int32_t on a specific target), edit XELPREG in
# xelpcfg.h (or your xelp_ovr.h).
#
# ---- Further reading ----
#
#   src/xelpcfg.h   -- all compile-time knobs with inline documentation
#   src/xelp.h      -- public API, version macro, struct layout
#   README.md       -- usage overview and examples
# =============================================================================

# ---- ESP-IDF component path ----
# ESP-IDF sets ESP_PLATFORM when it drives the build.  Register the
# component and return early -- none of the plain-CMake logic below
# applies inside ESP-IDF.
if(ESP_PLATFORM)
    idf_component_register(
        SRCS "src/xelp.c"
        INCLUDE_DIRS "src"
    )
    return()
endif()

# ---- Plain CMake library ----
cmake_minimum_required(VERSION 3.10)
project(xelp
    VERSION 0.3.4
    LANGUAGES C
    DESCRIPTION "Embedded CLI/script interpreter -- no malloc, multi-instance"
)

# xelp is a single-file C library.  Build as STATIC so parent projects
# can link it with target_link_libraries().
add_library(xelp STATIC
    src/xelp.c
)

# Public include directory -- consumers get src/ on their include path
# so #include "xelp.h" works (xelp.h pulls in xelpcfg.h automatically).
target_include_directories(xelp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# xelp is pure C89/C90 with no external dependencies.
set_target_properties(xelp PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

# ---------------------------------------------------------------------------
# Optional: unit tests (host builds only -- requires POSIX)
# ---------------------------------------------------------------------------
option(XELP_BUILD_TESTS "Build and register xelp unit tests" OFF)

if(XELP_BUILD_TESTS)
    enable_testing()

    add_executable(xelp_unit_tests
        tests/xelp_unit_tests.c
        tests/jumpbug_unit_test_fw.c
        src/xelp.c
    )
    target_include_directories(xelp_unit_tests PRIVATE src tests)
    add_test(NAME xelp_unit_tests COMMAND xelp_unit_tests)
endif()
