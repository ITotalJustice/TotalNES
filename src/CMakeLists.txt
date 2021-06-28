cmake_minimum_required(VERSION 3.13.4)

if (NES_SINGLE_FILE)
    add_library(TotalNES single.c)

    target_compile_definitions(TotalNES PRIVATE NES_SINGLE_FILE=1)
else()
    add_library(TotalNES
       bus.c
       cart.c
       cpu.c
       nes.c
       ppu.c
       joypad.c

       apu/apu.c
       apu/apu_io.c
       apu/square1.c
       apu/square2.c
       apu/triangle.c
       apu/noise.c
       apu/dmc.c
    
       mappers/mapper_000.c
       mappers/mapper_001.c
       mappers/mapper_002.c
       mappers/mapper_003.c
    )

    target_compile_definitions(TotalNES PRIVATE NES_SINGLE_FILE=0)
endif()

target_include_directories(TotalNES PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(TotalNES PRIVATE c_std_99)

if (NES_DEV)
    list(APPEND _flags
        -Wall
        -Wextra
        -Werror
        -Wfatal-errors
        -Wpedantic
        -Wshadow
        -Wdouble-promotion
        -Wformat=2
        -Wformat-overflow=2
        -Wformat-truncation=1
        -Wundef
        -Wmissing-include-dirs
        -Wstrict-aliasing=2
        -Walloca
        -Wduplicated-branches
        -Wduplicated-cond
        -Wwrite-strings
        -Wdate-time
        -Wsizeof-pointer-memaccess
        -Wlogical-op
        -Wpacked
        -Wredundant-decls
        -Wnested-externs
        # -Winline
        -ftrapv
    )

    list(APPEND _sanitizers
        # -fsanitize=address
        # -fsanitize=leak
        -fsanitize=undefined
        # -fsanitize=pointer-compare
        # -fsanitize=pointer-subtract
        -fstack-protector-all
    )

    # include(CheckCCompilerFlag)

    # TODO: check_c_compiler_flag() seems to not work on all flags
    # for some reason. example -Werror fails, even though its valid for gcc...
    
    foreach (flag IN LISTS _flags)
        # check_c_compiler_flag(${flag} CHECK_FLAG_${flag})
        # if (CHECK_FLAG_${flag})
            target_compile_options(TotalNES PRIVATE ${flag})
        # endif()
    endforeach()

    foreach (flag IN LISTS _sanitizers)
        # check_c_compiler_flag(${flag} CHECK_FLAG_${flag})
        # if (CHECK_FLAG_${flag})
            target_compile_options(TotalNES PRIVATE ${flag})
            target_link_options(TotalNES INTERFACE ${flag})
        # endif()
    endforeach()

endif()

if (NES_DEBUG)
    target_compile_definitions(TotalNES PRIVATE NES_DEBUG=1)
endif()
