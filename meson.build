project('totalnes', ['c', 'cpp'],
    version: '0.0.1',
    default_options: [
        'buildtype=debug',
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

    # used for testing
    'src/main.c',
])

includes = include_directories([
    'src',
])

cflags = [
    # optimise
    # '-Ofast',
    # '-flto',
    # '-DNDEBUG',

    # warns
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Wswitch-enum',
    '-Wduplicated-branches',
    '-Wduplicated-cond',
    '-Wshadow',
    '-Wfatal-errors',
    '-Wdouble-promotion',
    '-Wframe-larger-than=300KB',

    '-DNES_DEBUG',
]

cppflags = [
    '-std=gnu++17',
    '-Wuseless-cast',
    '-fdevirtualize-at-ltrans',
    '-Wsuggest-final-types',
]

linkflags = [
    # '-Wl,-subsystem,windows',
    # '-s',
]

# subdir('res')

executable(
    'totalnes',
    # res, # set the icon
    [ source ],
    dependencies : [ dependencies ] ,
    install: false,
    include_directories: [ includes ],
    c_args: [ cflags ],
    cpp_args: [ cppflags, cflags ],
    link_args: [ linkflags ]
)
