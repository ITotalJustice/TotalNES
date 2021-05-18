project('TotalNES', ['c'],
    version: '0.0.1',
    license : 'MIT',
    default_options: [
        # core options
        'buildtype=debug',
        'warning_level=3',
        'optimization=0',
        'strip=false',
        'werror=true',

        # base options
        'b_ndebug=if-release',
        'b_sanitize=address,undefined',

        'c_std=c99',
    ],
)

dependencies = [
    dependency('sdl2', version : '>=2.0.5'),
]

source = files([
    # core
    'src/core/bus.c',
    'src/core/cart.c',
    'src/core/cpu.c',
    'src/core/nes.c',
    'src/core/ppu.c',
    'src/core/joypad.c',
    'src/core/apu/apu.c',
    'src/core/apu/apu_io.c',
    'src/core/apu/square1.c',
    'src/core/apu/square2.c',
    'src/core/apu/triangle.c',
    'src/core/apu/noise.c',
    'src/core/apu/dmc.c',

    # used for testing
    'src/main.c',
])

includes = include_directories([
    'src',
])

c_flags = [
   '-DNES_DEBUG', 
]

c_warnings = [
  '-Wall',
  '-Wextra',
  '-Werror',
  '-Wfatal-errors',
  '-Wpedantic',
  '-Wshadow',
  '-Wdouble-promotion',
  '-Wformat=2',
  '-Wformat-overflow=2',
  '-Wformat-truncation=1',
  '-Wundef',
  '-Wmissing-include-dirs',
  '-Wstrict-aliasing=2',
  '-Walloca',
  '-Wduplicated-branches',
  '-Wduplicated-cond',
  '-Wwrite-strings',
  '-Wdate-time',
  '-Wsizeof-pointer-memaccess',
  '-Wlogical-op',
  '-Wpacked',
  '-Wredundant-decls',
  '-Wnested-externs',
  '-Winline',
  '-Wcast-qual',
  '-Wcast-align',
  '-Wpointer-arith',
  '-Winit-self',
  '-Wswitch-enum',
  '-Wstrict-prototypes',
  #'-Wmissing-prototypes',
  '-Wredundant-decls',
  '-Wfloat-equal',
  '-Wvla',
  #'-Wc++-compat',
  #'-Wconversion',

  '-fsanitize=address',
  '-fsanitize=leak',
  '-fsanitize=undefined',
  '-fsanitize=pointer-compare',
  '-fsanitize=pointer-subtract',
  '-fstack-protector-all',
]

linkflags = [
    '-fsanitize=address',
    '-fsanitize=leak',
    '-fsanitize=undefined',
    '-fsanitize=pointer-compare',
    '-fsanitize=pointer-subtract',
    '-fstack-protector-all',
]

exe = executable(
    'TotalNES',
    [ source ],
    dependencies : [ dependencies ],
    install: true,
    include_directories: [ includes ],
    c_args: [ c_warnings, c_flags ],
    link_args: [ linkflags ],
)

# scripts
run_target('cppcheck',
  command : 'scripts/cppcheck.sh')
