cmake_minimum_required(VERSION 3.13.4)


project(TotalNES
    VERSION 0.0.1
    DESCRIPTION "WIP NES emulator written in c99!"
    HOMEPAGE_URL "https://github.com/ITotalJustice/TotalNES"
    LANGUAGES C
)

option(NES_SINGLE_FILE "include all src in single.c" OFF)
option(NES_DEBUG "enable debug" OFF)
option(NES_DEV "enables debug and sanitizers" OFF)

option(NES_EXAMPLE_ALL "builds all examples" OFF)

option(NES_TEST_ROMRUNNER "" OFF)
# option(NES_TEST_SANITY "build test that ensures the lib can be built" OFF)
# option(NES_TEST_BENCH "" OFF)
option(NES_TEST_ALL "build all tests" OFF)


if (NES_EXAMPLE_ALL)
endif()

if (NES_DEV)
    set(NES_DEBUG ON)
endif()

if (NES_TEST_ALL)
    set(NES_TEST_ROMRUNNER ON)
endif()

add_subdirectory(src)
# add_subdirectory(tests)
add_subdirectory(examples)
