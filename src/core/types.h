#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "core/mappers.h"


enum NesError {
    NES_UNSUPORTED_MAPPER = -10,
    NES_UNKNOWN_HEADER = -9,
    NES_BAD_ROM = -8,
    NES_OK = 0
};


// fwd;
struct NES_INES;
struct NES_INES2;
struct NES_CartHeader;
struct NES_Cpu;
struct NES_Ppu;
struct NES_Cart;
struct NES_Core;


/* APU START */
struct NES_Pulse {
    uint8_t duty : 2;
    uint8_t length_counter_halt : 1;
    uint8_t constant_volume : 1;
    uint8_t volume : 4;

    uint8_t sweep : 1;
    uint8_t period : 3;
    uint8_t negate : 1;
    uint8_t shift : 3;

    uint8_t timer_lower : 8;

    uint8_t length_counter_load : 5;
    uint8_t timer_high : 3;
};

struct NES_Triangle {
    uint8_t length_counter_halt : 1;
    uint8_t linear_counter_load : 7;
    uint8_t : 8;
    uint8_t timer_low : 8;
    uint8_t length_counter_load : 5;
    uint8_t timer_high : 3;
};

struct NES_Noise {
    uint8_t : 2;
    uint8_t length_counter_halt : 1;
    uint8_t constant_volume : 1;
    uint8_t volume : 4;

    uint8_t : 8;

    uint8_t loop_noise : 1;
    uint8_t : 3;
    uint8_t noise_period : 4;

    uint8_t length_counter_load : 5;
    uint8_t : 3;
};

struct NES_Dmc {
    uint8_t irq_enable : 1;
    uint8_t loop : 1;
    uint8_t : 2;
    uint8_t freq : 4;

    uint8_t : 1;
    uint8_t load_counter : 7;

    uint8_t sample_address : 8;

    uint8_t sample_length : 8;
};

struct NES_Apu {
    union {
        uint8_t io[0x18];

        struct {
            struct NES_Pulse pulse1;
            struct NES_Pulse pulse2;
            struct NES_Triangle triangle;
            struct NES_Noise noise;
            uint8_t _pad0[1];
            
            struct {
                uint8_t dmc_irq : 1;
                uint8_t frame_irq : 1;
                uint8_t : 1;
                uint8_t enable_dmc : 1;
                uint8_t noise : 1;
                uint8_t triangle : 1;
                uint8_t pulse2 : 1;
                uint8_t pulse1 : 1;
            } status;

            uint8_t _pad1[1];
            
            struct {
                uint8_t mode : 1;
                uint8_t irq : 1;
                uint8_t : 6;
            } frame_counter;
        };
    };
};
/* APU END */


/* PPU START */
enum NesScreenSize {
    NES_SCREEN_WIDTH = 256,
    NES_SCREEN_HEIGHT = 240,
};

struct NES_Sprite {
    uint8_t y;

    struct {
        uint8_t bank : 1;
        uint8_t top : 7;
    } num;

    struct {
        uint8_t palette : 2;
        uint8_t : 3;
        uint8_t priority : 1;
        uint8_t flipx : 1;
        uint8_t flipy : 1;
    } attr;
    
    uint8_t x;
};

struct NES_Colour { /* bgr */
    uint8_t r : 5;
    uint8_t g : 5;
    uint8_t b : 5;
    uint8_t a : 1; /* unused, here for padding */
};

struct NES_Ppu {
    union {
        struct {
            uint8_t nametable : 2;
            uint8_t vram_addr : 1;
            uint8_t obj_8x8_addr : 1;
            uint8_t bg_addr : 1;
            uint8_t obj_size : 1;
            uint8_t master : 1;
            uint8_t nmi : 1;
        } _ctrl;
        uint8_t ctrl;
    };

    union {
        struct {
            uint8_t greyscale : 1;
            uint8_t bg_leftmost : 1;
            uint8_t obj_leftmost : 1;
            uint8_t bg_on : 1;
            uint8_t obj_on : 1;
            uint8_t bgr : 3;
        } _mask;
        uint8_t mask;
    };

    union {
        struct {
            uint8_t lsb : 5;
            uint8_t obj_overflow : 1;
            uint8_t obj_hit : 1;
            uint8_t vblank : 1;
        } _status;
        uint8_t status;
    };

    uint16_t addr;
    uint8_t oam_addr;
    uint8_t scroll;
    uint8_t data;
    uint8_t dma;
    uint8_t latch;
    uint8_t addr_inc_value;
    uint8_t pram[0x20]; /* palette ram */
    uint8_t oam[0x100]; /* object attribute memory */
    uint8_t vram[0x800]; /* video ram */

    /* framebuffer */
    uint16_t pixels[NES_SCREEN_HEIGHT][NES_SCREEN_WIDTH];
};
/* PPU END */

/* CART START */
struct NES_INES {
    struct {
        uint8_t pgr_ram_size;
    } flag8;
    
    struct {
        uint8_t tv_system : 1;
        uint8_t : 7;
    } flag9;
    
    uint8_t unused[0x6];
};

struct NES_INES2 {
    struct {
        uint8_t mapper_num : 4;
        uint8_t submapper_num : 4;
    } flag8;
    
    struct {
        uint8_t pgr_rom : 4;
        uint8_t chr_rom : 4;
    } flag9;
    
    struct {
        uint8_t pgr_ram : 4;
        uint8_t pgr_nvram : 4;
    } flag10;
    
    struct {
        uint8_t chr_ram : 4;
        uint8_t chr_nvram : 4;
    } flag11;
    
    struct {
        uint8_t timing_mode : 2;
        uint8_t : 6;
    } flag12;
    
    union { /* todo :  name the structs  */
        struct {
            uint8_t ppu_type : 4;
            uint8_t hw_type : 4;
        } a;
        struct {
            uint8_t extended_console_type : 4;
            uint8_t : 4;
        } b;
    } flag13;
    
    struct {
        uint8_t idk : 2;
        uint8_t : 6;
    } flag14;

    struct {
        uint8_t default_expansion_device : 6;
        uint8_t : 2;
    } flag15;
};

struct NES_CartHeader {
    uint8_t header_id[0x4];
    uint8_t pgr_rom_size; /* x16k */
    uint8_t chr_rom_size; /* x8k */

    struct {
        uint8_t nametable_table : 1;
        uint8_t battery : 1;
        uint8_t trainer : 1;
        uint8_t four_screen_mode : 1;
        uint8_t mapper_num_lo : 4;
    } flag6;
    
    struct {
        uint8_t console_type : 2;
        uint8_t nes2 : 2;
        uint8_t mapper_num_hi : 4;
    } flag7;

    union {
        struct NES_INES ines;
        struct NES_INES2 ines2;
    };
};

struct NES_Cart {
    /* callbacks */
    uint8_t (*mapper_read)(struct NES_Core*, uint16_t);
    void (*mapper_write)(struct NES_Core*, uint16_t, uint8_t);
    
    uint8_t* pgr_rom;
    uint8_t* chr_rom;

    union {
        struct NES_Mapper_000 mapper_000;
        struct NES_Mapper_001 mapper_001;
        struct NES_Mapper_002 mapper_002;
        struct NES_Mapper_003 mapper_003;
        struct NES_Mapper_004 mapper_004;
        struct NES_Mapper_005 mapper_005;
        struct NES_Mapper_009 mapper_009;
        struct NES_Mapper_010 mapper_010;
        struct NES_Mapper_037 mapper_037;
        struct NES_Mapper_047 mapper_047;
        struct NES_Mapper_066 mapper_066;
        struct NES_Mapper_099 mapper_099;
        struct NES_Mapper_105 mapper_105;
    };

    uint32_t pgr_rom_size;
    uint32_t pgr_ram_size;
    uint32_t chr_rom_size;
    uint32_t chr_ram_size;

    uint8_t* base_rom; /* rom buffer passed into loadrom() */
};
/* CART END */

struct NES_Cpu {
    uint16_t cycles;
    uint16_t PC; /* program counter */
    uint8_t A; /* https : //youtu.be/dBK0gKW61NU?t=221 */
    uint8_t X; /* index X */
    uint8_t Y; /* index Y */
    uint8_t S; /* stack pointer */

    union {
        struct {
            uint8_t C : 1; /* carry */
            uint8_t Z : 1; /* zero */
            uint8_t I : 1; /* interrupt disable */
            uint8_t D : 1; /* decimal */
            uint8_t B : 2; /* <no effect> */
            uint8_t V : 1; /* overflow */
            uint8_t N : 1; /* negative */
        } status;
        uint8_t P; /* status */
    };
};

struct NES_Joypad {
    uint8_t a;
    uint8_t b;
};

struct NES_Core {
    struct NES_Cpu cpu;
    struct NES_Apu apu;
    struct NES_Ppu ppu;
    struct NES_Joypad jp;
    struct NES_Cart cart;
    uint8_t ram[0x800]; // 2kb
};


#ifdef __cplusplus
}
#endif
