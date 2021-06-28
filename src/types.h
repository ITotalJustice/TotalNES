#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NES_DEBUG
    #define NES_DEBUG 0
#endif

#ifndef NES_SINGLE_FILE
    #define NES_SINGLE_FILE 0
#endif

#if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_LIB
        #define NESAPI __declspec(dllexport)
    #else
        #define NESAPI __declspec(dllimport)
    #endif
#else
    #ifdef BUILDING_LIB
        #define NESAPI __attribute__ ((visibility ("default")))
    #else
        #define NESAPI
    #endif
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


// fwd;
struct NES_INES;
struct NES_INES2;
struct NES_CartHeader;
struct NES_Cpu;
struct NES_Ppu;
struct NES_Cart;
struct NES_Core;
struct NES_ApuCallbackData;


typedef void(*nes_apu_callback_t)(void* user, struct NES_ApuCallbackData* data);
typedef void(*nes_vblank_callback_t)(void* user);


enum
{
    // 4MiB but most roms are a lot less.
    // much like my gb emu, should have a rom info func which tells
    // the user how much rom / ram space is needed.
    NES_ROM_SIZE_MAX = 1024 * 1024 * 4,

    NES_SCREEN_WIDTH = 256,
    NES_SCREEN_HEIGHT = 240,
};


/* APU START */
struct NES_Square
{
    uint8_t volume;
    bool constant_volume;
    bool length_counter_halt;
    uint8_t duty; // 0-3

    uint8_t sweep_shift;
    uint8_t sweep_negate;
    uint8_t sweep_period;
    uint8_t sweep_enabled;

    uint8_t duty_index; // 0-7

    uint16_t timer_reload;

    int16_t timer;
    int16_t sweep_counter;
    int16_t length_counter;
    int8_t envelope_counter;
};

struct NES_Triangle
{
    uint8_t linear_counter_load ;
    bool length_counter_halt;

    uint8_t duty_index; // 0-31

    uint16_t timer_reload;

    int16_t timer;
    int16_t length_counter;
};

struct NES_Noise
{
    uint8_t volume;
    bool constant_volume;
    bool length_counter_halt;

    uint8_t frequency;
    uint8_t random_number_mode;

    uint16_t lsfr;

    int16_t timer;
    int16_t length_counter;
    int8_t envelope_counter;
};

struct NES_Status
{
    bool dmc_irq;
    bool frame_irq;
    bool dmc_enable;
    bool noise_enable;
    bool triangle_enable;
    bool square2_enable;
    bool square1_enable;
};

struct NES_FrameSequencer
{
    uint8_t mode;
    bool irq_enable;

    // either 0-4 or 0-5 depending on the mode
    uint8_t step;

    int16_t timer;
};

struct NES_ApuCallbackData
{
    int8_t square1;
    int8_t square2;
    int8_t triangle;
    int8_t noise;
};

struct NES_Apu
{
    // reading from apu io returns the last written value
    // because of this, it is stored in an array
    uint8_t io[0x18];

    struct NES_Square square1;
    struct NES_Square square2;
    struct NES_Triangle triangle;
    struct NES_Noise noise;

    struct NES_Status status;
    struct NES_FrameSequencer frame_sequencer;
};
    /* APU END */


    /* PPU START */
struct NES_Ppu
{
    uint8_t* map[0x10];

    uint8_t ctrl;
    uint8_t mask;
    uint8_t status;

    uint16_t vram_addr;
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
    bool has_first_8bit;

    // cpu vram reads are buffered, so theres a 1-byte delay
    uint8_t vram_latched_read;

    // the vram is incremented by either 1 or 32 after each write.
    // this value is set after the cpu writes to $2000
    uint8_t vram_addr_increment;

    uint16_t cycles; // 0-341
    int16_t next_cycles;
    int16_t scanline; // -1 - 261

    uint8_t pram[32]; /* palette ram */
    uint8_t oam[256]; /* object attribute memory */
    uint8_t vram[1024 * 2]; /* video ram */
};
    /* PPU END */

    /* CART START */
    // TODO: i want to remove bitfields here, but it would mean adding
    // in *a lot* of extra code for the header stuff, which doesn't
    // need doing just yet.
    // this will fixed at some point still.
enum NesMapperType
{
    NesMapperType_000, // https://wiki.nesdev.com/w/index.php/INES_Mapper_000
    NesMapperType_001, // https://wiki.nesdev.com/w/index.php/INES_Mapper_001
    NesMapperType_002, // https://wiki.nesdev.com/w/index.php/INES_Mapper_002
    NesMapperType_003, // https://wiki.nesdev.com/w/index.php/INES_Mapper_003
    NesMapperType_004, // https://wiki.nesdev.com/w/index.php/INES_Mapper_004
};

// https://wiki.nesdev.com/w/index.php/INES_Mapper_000
struct NES_Mapper_000
{
    const uint8_t* prg_rom_slots[2];
    uint8_t* prg_ram_slots[2];
    uint8_t* chr_ram_slots[2];
    uint8_t prg_ram[1024 * 4];  // 4k
    uint8_t chr_ram[1024 * 8];  // 8k
};

// https://wiki.nesdev.com/w/index.php/INES_Mapper_001
struct NES_Mapper_001
{
    const uint8_t* prg_rom_slots[2];
    uint8_t* prg_ram_slots[2];
    uint8_t* chr_ram_slots[2];

    uint8_t prg_ram[1024 * 8]; // 8k
    uint8_t chr_ram[1024 * 128];  // 128k

    uint8_t shift_reg; // 5-bit
    uint8_t shift_reg_count; // counts to 5

    // control
    uint8_t mirroring;
    uint8_t prg_rom_bank_mode;
    uint8_t chr_rom_bank_mode;

    uint8_t chr_bank0;
    uint8_t chr_bank1;

    uint8_t prg_bank;
    bool prg_ram_enable;
};

// https://wiki.nesdev.com/w/index.php/INES_Mapper_002
struct NES_Mapper_002
{
    const uint8_t* prg_rom_slots[2];
    uint8_t* chr_ram_slots[2];
    uint8_t chr_ram[1024 * 8];  // 8k
    // uses bits 0-2 to select bank for 0x8000-0xFFFF
    uint8_t bank_select;
};

// https://wiki.nesdev.com/w/index.php/INES_Mapper_003
struct NES_Mapper_003
{
    const uint8_t* prg_rom_slots[2];
    uint8_t chr_rom[1024 * 32];  // 8k
    // uses bits 0-2 to select bank for 0x8000-0xFFFF
    uint8_t bank_select;
};

// https://wiki.nesdev.com/w/index.php/INES_Mapper_004
struct NES_Mapper_004
{
    uint8_t prg_ram[1024 * 8];  // 8k
    uint8_t chr_ram[1024 * 8];  // 8k
};

struct NES_INES
{
    struct
    {
        uint8_t pgr_ram_size;
    } flag8;

    struct
    {
        uint8_t tv_system : 1;
        uint8_t : 7;
    } flag9;

    uint8_t unused[0x6];
};

struct NES_INES2
{
    struct
    {
        uint8_t mapper_num : 4;
        uint8_t submapper_num : 4;
    } flag8;

    struct
    {
        uint8_t pgr_rom : 4;
        uint8_t chr_rom : 4;
    } flag9;

    struct
    {
        uint8_t pgr_ram : 4;
        uint8_t pgr_nvram : 4;
    } flag10;

    struct
    {
        uint8_t chr_ram : 4;
        uint8_t chr_nvram : 4;
    } flag11;

    struct
    {
        uint8_t timing_mode : 2;
        uint8_t : 6;
    } flag12;

    union
    { /* todo :  name the structs  */
        struct
        {
            uint8_t ppu_type : 4;
            uint8_t hw_type : 4;
        } a;
        struct
        {
            uint8_t extended_console_type : 4;
            uint8_t : 4;
        } b;
    } flag13;

    struct
    {
        uint8_t idk : 2;
        uint8_t : 6;
    } flag14;

    struct
    {
        uint8_t default_expansion_device : 6;
        uint8_t : 2;
    } flag15;
};

struct NES_CartHeader
{
    uint8_t header_id[0x4];
    uint8_t pgr_rom_size; /* 16k */
    uint8_t chr_rom_size; /* 8k */

    struct
    {
        uint8_t nametable_table : 1;
        uint8_t battery : 1;
        uint8_t trainer : 1;
        uint8_t four_screen_mode : 1;
        uint8_t mapper_num_lo : 4;
    } flag6;

    struct
    {
        uint8_t console_type : 2;
        uint8_t nes2 : 2;
        uint8_t mapper_num_hi : 4;
    } flag7;

    union
    {
        struct NES_INES ines;
        struct NES_INES2 ines2;
    } type;
};

struct NES_Cart
{
    const uint8_t* pgr_rom;
    const uint8_t* chr_rom;

    enum NesMapperType mapper_type;
    
    union
    {
        struct NES_Mapper_000 _000;
        struct NES_Mapper_001 _001;
        struct NES_Mapper_002 _002;
        struct NES_Mapper_003 _003;
        struct NES_Mapper_004 _004;
    } mapper;

    uint32_t pgr_rom_size;
    uint32_t pgr_ram_size;
    uint32_t chr_rom_size;
    uint32_t chr_ram_size;
};
    /* CART END */

struct NES_Cpu
{
    uint16_t cycles;
    uint16_t PC; /* program counter */
    uint8_t A; /* https://youtu.be/dBK0gKW61NU?t=221 */
    uint8_t X; /* index X */
    uint8_t Y; /* index Y */
    uint8_t S; /* stack pointer */

    bool C; /* carry */
    bool Z; /* zero */
    bool I; /* interrupt disable */
    bool D; /* decimal */
    bool B; /* <no effect> */
    bool V; /* overflow */
    bool N; /* negative */
};

struct NES_Joypad
{
    bool strobe;

    uint8_t buttons_a; // which buttons are down etc
    uint8_t buttons_b; // which buttons are down etc

    uint8_t latch_a; // $4016
    uint8_t latch_b; // $4017
};

struct NES_Palette
{
    uint32_t colour[64];
};

struct NES_Core
{
    struct NES_Cpu cpu;
    struct NES_Apu apu;
    struct NES_Ppu ppu;
    struct NES_Joypad jp;
    struct NES_Cart cart;
    struct NES_Palette palette;
    uint8_t wram[1024 * 2];

    const uint8_t* rom; /* rom buffer passed into loadrom() */
    size_t rom_size;

    void* pixels;
    uint32_t pixels_stride;
    uint8_t bpp;
    
    nes_vblank_callback_t vblank_callback;
    void* vblank_callback_user;

    nes_apu_callback_t apu_callback;
    void* apu_callback_user;

    uint32_t apu_callback_freq;
    int32_t apu_callback_counter;
};

#ifdef __cplusplus
}
#endif
