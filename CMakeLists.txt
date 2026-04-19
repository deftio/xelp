# CMakeLists.txt -- ESP-IDF component build definition for xelp.
#
# This file is used by the ESP-IDF build system (CMake-based) when xelp
# is installed as a component via the ESP Component Registry or placed
# in a project's components/ directory.
#
# The idf_component_register() call is specific to ESP-IDF and is
# silently ignored by plain CMake, so this file does not interfere
# with other build systems (make, PlatformIO, etc.).
#
# Usage in an ESP-IDF project:
#
#   1. Install:  idf.py add-dependency "deftio/xelp==0.3.0"
#      -- or --  copy this repo into your project's components/xelp/
#
#   2. In your source:
#        #include "xelp.h"
#
#   3. Build:    idf.py build
#
# No REQUIRES or PRIV_REQUIRES -- xelp depends only on standard C
# (C89 or later) and has no ESP-IDF component dependencies.

idf_component_register(
    SRCS "src/xelp.c"
    INCLUDE_DIRS "src"
)
