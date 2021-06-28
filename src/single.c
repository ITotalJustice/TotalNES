#include "types.h"


#if NES_SINGLE_FILE
    #include "apu/apu.c"
    #include "apu/apu_io.c"
    #include "apu/dmc.c"
    #include "apu/noise.c"
    #include "apu/square1.c"
    #include "apu/square2.c"
    #include "apu/triangle.c"
    #include "bus.c"
    #include "cart.c"
    #include "cpu.c"
    #include "joypad.c"
    #include "nes.c"
    #include "ppu.c"
    #include "mappers/mapper_000.c"
    #include "mappers/mapper_001.c"
    #include "mappers/mapper_002.c"
    #include "mappers/mapper_003.c"
#endif
