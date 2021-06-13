cmake_minimum_required(VERSION 3.13.4)

if (NES_EXAMPLE_SDL)
    add_executable(example_sdl example_sdl.c)
    target_link_libraries(example_sdl LINK_PRIVATE TotalNES)

    find_package(SDL2 REQUIRED)
    target_include_directories(example_sdl PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(example_sdl PRIVATE ${SDL2_LIBRARIES})
endif()
