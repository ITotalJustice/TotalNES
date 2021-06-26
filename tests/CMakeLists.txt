cmake_minimum_required(VERSION 3.13.4)


if (NES_TEST_AUDIO)
    add_executable(test_audio test_audio.c)
    target_link_libraries(test_audio LINK_PRIVATE TotalNES)

    find_package(SDL2 REQUIRED)
    target_include_directories(test_audio PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(test_audio PRIVATE ${SDL2_LIBRARIES})
endif()

if (NES_TEST_GFX)
    add_executable(test_gfx test_gfx.c)
    target_link_libraries(test_gfx LINK_PRIVATE TotalNES)

    find_package(SDL2 REQUIRED)
    target_include_directories(test_gfx PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(test_gfx PRIVATE ${SDL2_LIBRARIES})
endif()
