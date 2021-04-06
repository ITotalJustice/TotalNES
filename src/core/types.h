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
struct NES_Square {
    uint8_t volume : 4;
    uint8_t constant_volume : 1;
    uint8_t length_counter_halt : 1;
    uint8_t duty : 2; // 0-3

    uint8_t sweep_shift : 3;
    uint8_t sweep_negate : 1;
    uint8_t sweep_period : 3;
    uint8_t sweep_enabled : 1;

    uint8_t duty_index : 3; // 0-7

    uint16_t timer_reload;

    int16_t timer;
    int16_t sweep_counter;
    int16_t length_counter;
    int8_t envelope_counter;
};

struct NES_Triangle {
    uint8_t linear_counter_load : 7;
    uint8_t length_counter_halt : 1;

    uint8_t duty_index : 5; // 0-31

    uint16_t timer_reload;

    int16_t timer;
    int16_t length_counter;
};

struct NES_Noise {
    uint8_t volume : 4;
    uint8_t constant_volume : 1;
    uint8_t length_counter_halt : 1;

    uint8_t frequency : 4;
    uint8_t random_number_mode :1;

    uint16_t lsfr : 15;

    int16_t timer;
    int16_t length_counter;
    int8_t envelope_counter;
};

struct NES_Status {
    uint8_t dmc_irq : 1;
    uint8_t frame_irq : 1;
    uint8_t dmc_enable : 1;
    uint8_t noise_enable : 1;
    uint8_t triangle_enable : 1;
    uint8_t square2_enable : 1;
    uint8_t square1_enable : 1;
};

struct NES_FrameSequencer {
    uint8_t mode : 1;
    uint8_t irq_enable : 1;

    // either 0-4 or 0-5 depending on the mode
    uint8_t step : 3;

    int16_t timer;
};

struct NES_ApuCallbackData {
    // this number is arbitrary, though should be small enough that
    // there's minimal latency!
	int8_t samples[512];
};

struct NES_Apu {
    // reading from apu io returns the last written value
    // because of this, it is stored in an array
    uint8_t io[0x18];

    struct NES_Square square1;
    struct NES_Square square2;
    struct NES_Triangle triangle;
    struct NES_Noise noise;

    struct NES_Status status;
    struct NES_FrameSequencer frame_sequencer;

    int16_t sample_cycles;
    uint16_t sample_index;
    struct NES_ApuCallbackData sample_data;
};
/* APU END */


/* PPU START */
enum NesScreenSize {
    NES_SCREEN_WIDTH = 256,
    NES_SCREEN_HEIGHT = 240,
};

struct NES_Colour { /* BGR555 */
    uint8_t r : 5;
    uint8_t g : 5;
    uint8_t b : 5;
    uint8_t a : 1; /* unused alpha channel, here for padding */
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

    uint16_t vram_addr : 14;
    uint8_t oam_addr;

    // this is the value written to by $2005 AND $2006 first write
    uint8_t horizontal_scroll_origin;
    // this s the second byte of $2005
    uint8_t vertical_scroll_origin;

    // cpu writes to $2005 and $2006 require two 8-bit values
    // for the full value.
    // this value is shared between both registers, and is reset after
    // the second write and also after reading from $2000
    // NOTE, it seems this value is actually stored in $2005 first write.
    // does this mean that reading from the status register destroys this value?
    uint8_t write_flipflop;
    bool has_first_8bit : 1;

    // cpu vram reads are buffered, so theres a 1-byte delay
    uint8_t vram_latched_read;

    // the vram is incremented by either 1 or 32 after each write.
    // this value is set after the cpu writes to $2000
    uint8_t vram_addr_increment : 6;

    uint16_t cycles; // 0-341
    int16_t next_cycles;
    int16_t scanline; // -1 - 261

    uint8_t pram[0x20]; /* palette ram */
    uint8_t oam[0x100]; /* object attribute memory */
    uint8_t vram[0x800]; /* video ram */

    /* framebuffer */
    uint32_t pixels[NES_SCREEN_HEIGHT][NES_SCREEN_WIDTH];
};
/* PPU END */

/* CART START */
enum NesMapperType {
    NES_MAPPER_000,
};

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
    uint8_t pgr_rom_size; /* 16k */
    uint8_t chr_rom_size; /* 8k */

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
    uint8_t* pgr_rom;
    uint8_t* chr_rom;
    uint8_t* base_rom; /* rom buffer passed into loadrom() */

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

    enum NesMapperType mapper_type;

    uint32_t pgr_rom_size;
    uint32_t pgr_ram_size;
    uint32_t chr_rom_size;
    uint32_t chr_ram_size;
};
/* CART END */

struct NES_Cpu {
    uint16_t cycles;
    uint16_t PC; /* program counter */
    uint8_t A; /* https://youtu.be/dBK0gKW61NU?t=221 */
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
    bool strobe : 1;

    uint8_t buttons_a; // which buttons are down etc
    uint8_t buttons_b; // which buttons are down etc

    uint8_t latch_a; // $4016
    uint8_t latch_b; // $4017
};

struct GB_MixerData {
    int8_t square1;
    int8_t square2;
    int8_t triangle;
    int8_t noise;
};

typedef void(*NES_apu_callback_t)(struct NES_Core* nes, void* user,
    struct NES_ApuCallbackData* data
);

typedef int8_t(*NES_mixer_callback_t)(struct NES_Core* nes, void* user,
    struct GB_MixerData data
);


struct NES_Core {
    struct NES_Cpu cpu;
    struct NES_Apu apu;
    struct NES_Ppu ppu;
    struct NES_Joypad jp;
    struct NES_Cart cart;
    uint8_t ram[0x800]; // 2kb

    NES_mixer_callback_t mixer_cb;
    void* mixer_cb_user_data;

    NES_apu_callback_t apu_cb;
    void* apu_cb_user_data;
};


#ifdef __cplusplus
}
#endif
