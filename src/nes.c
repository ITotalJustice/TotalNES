#include "nes.h"
#include "internal.h"

#include <string.h>
#include <assert.h>


bool NES_init(struct NES_Core* nes)
{
    if (!nes)
    {
        return false;
    }

    memset(nes, 0, sizeof(struct NES_Core));

    return true;
}

void NES_reset(struct NES_Core* nes)
{
    nes->cpu.A = 0;
    nes->cpu.X = 0;
    nes->cpu.Y = 0;
    nes->cpu.S = 0xFD;
    nes->cpu.PC = 0; // this gets set when the rom is loaded

    nes->cpu.I = 1;
    nes->cpu.D = 0;
    nes->cpu.B = 2;
}

bool NES_is_header_valid(const struct NES_CartHeader* header)
{
    const uint8_t ines_header =
        header->header_id[0] == 'N' &&
        header->header_id[1] == 'E' &&
        header->header_id[2] == 'S' &&
        header->header_id[3] == 0x1A;

    if (!ines_header)
    {
        return false;
    }

    const uint8_t ines_header2 = header->flag7.nes2 == 0x3;

    if (ines_header2)
    {
        NES_log_err("ines 2.0 header\n");
        return false;
    }

    return true;
}

static uint8_t get_mapper_num_from_header(const struct NES_CartHeader* header)
{
    return (header->flag7.mapper_num_hi << 4) | header->flag6.mapper_num_lo;
}

static void log_header(const struct NES_CartHeader* header)
{
    // nametable_table
    NES_log("\nHEADER INFO\n");

    NES_log("\tmapper num: %u\n", get_mapper_num_from_header(header));
    NES_log("\tprg_rom: 0x%04X\n", header->prg_rom_size * 0x4000);
    NES_log("\tchr_rom: 0x%04X\n", header->chr_rom_size * 0x2000);
    NES_log("\tnametable_table: %u\n", header->flag6.nametable_table);
    NES_log("\tbattery: %u\n", header->flag6.battery);
    NES_log("\ttrainer: %u\n", header->flag6.trainer);
    NES_log("\tfour_screen_mode: %u\n", header->flag6.four_screen_mode);

    NES_log("\n");
}

static const struct NES_CartHeader* get_header(const uint8_t* rom, size_t size)
{
    if (!size || size < sizeof(struct NES_CartHeader))
    {
        NES_log_err("invalid rom size\n");
        return NULL;
    }

    return (const struct NES_CartHeader*)rom;
}

bool NES_get_rom_info(const uint8_t* rom, size_t size, struct NES_RomInfo* info)
{
    const struct NES_CartHeader* header = get_header(rom, size);

    if (!header)
    {
        return false;
    }

    if (!NES_is_header_valid(header))
    {
        return false;
    }

    const uint8_t mapper_num = get_mapper_num_from_header(header);
    
    // this will fail if we don't have impl for the mapper yet!
    if (!nes_mapper_get_prg_chr_ram_size(mapper_num, &info->prg_ram_size, &info->chr_ram_size))
    {
        NES_log_err("failed to get prg_chr size from mapper: %u\n", mapper_num);
        return false;
    }

    // if this has a value, then rom is included so ram isn't needed!
    if (header->chr_rom_size)
    {
        info->chr_ram_size = 0;//header->chr_rom_size * (1024 * 8);
    }

    info->prg_battery = header->flag6.battery;
    info->chr_battery = false;

    return true;
}

bool NES_loadrom(struct NES_Core* nes, const uint8_t* rom, size_t size)
{
    // todo: this should happen much further down, after rom is ready
    // to be loaded!
    NES_reset(nes);

    const struct NES_CartHeader* header = get_header(rom, size);

    if (!header)
    {
        return false;
    }

    const uint8_t ines_header =
        header->header_id[0] == 'N' &&
        header->header_id[1] == 'E' &&
        header->header_id[2] == 'S' &&
        header->header_id[3] == 0x1A;

    if (!ines_header)
    {
        return false;
    }

    /* todo: support ines 2.0  */
    const uint8_t ines_header2 = header->flag7.nes2 == 0x3;

    if (ines_header2)
    {
        NES_log_fatal("ines 2.0 header\n");
        return false;
    }

    log_header(header);

    const uint8_t mapper_num = (header->flag7.mapper_num_hi << 4) | header->flag6.mapper_num_lo;

    enum Mirror mirror = HORIZONTAL;

    if (header->flag6.nametable_table)
    {
        mirror = VERTICAL;
    }

    if (header->flag6.four_screen_mode)
    {
        mirror = FOUR_SCREEN;
    }

    const uint32_t prg_rom_size = header->prg_rom_size * 0x4000;
    const uint32_t chr_rom_size = header->chr_rom_size * 0x2000;

    /* seek past header and trainer (if any). */
    rom += sizeof(struct NES_CartHeader) + (header->flag6.trainer * 0x200);

    nes->cart.prg_rom = rom;
    nes->cart.chr_rom = rom + prg_rom_size;
    nes->cart.prg_rom_size = prg_rom_size;
    nes->cart.chr_rom_size = chr_rom_size;

    if (!nes_mapper_setup(nes, mapper_num, mirror))
    {
        NES_log_err("MISSING MAPPER: %u\n", mapper_num);
        return false;
    }

    // load from the reset vector
    nes->cpu.PC = nes_cpu_read16(nes, VECTOR_RESET);

    nes_apu_init(nes);
    nes_ppu_init(nes);

    return true;
}

void NES_set_prg_ram(struct NES_Core* nes, uint8_t* data, size_t size)
{
    nes->prg_ram = data;
    nes->prg_ram_size = size;
}

void NES_set_chr_ram(struct NES_Core* nes, uint8_t* data, size_t size)
{
    nes->chr_ram = data;
    nes->chr_ram_size = size;
}

void NES_set_pixels(struct NES_Core* nes, void* pixels, uint32_t stride, uint8_t bpp)
{
    nes->pixels = pixels;
    nes->pixels_stride = stride;
    nes->bpp = bpp;
}

void NES_set_palette(struct NES_Core* nes, const struct NES_Palette* palette)
{
    memcpy(&nes->palette, palette, sizeof(nes->palette));
}

void NES_set_apu_callback(struct NES_Core* nes, nes_apu_callback_t cb, void* user, uint32_t freq)
{
    nes->apu_callback = cb;
    nes->apu_callback_user = user;
    nes->apu_callback_freq = freq;
}

void NES_set_vblank_callback(struct NES_Core* nes, nes_vblank_callback_t cb, void* user)
{
    nes->vblank_callback = cb;
    nes->vblank_callback_user = user;
}

void NES_step(struct NES_Core* nes)
{
    nes->cpu.cycles = 0;

    nes_cpu_run(nes);
    nes_ppu_run(nes, nes->cpu.cycles);
    nes_apu_run(nes, nes->cpu.cycles);
}

void NES_run_frame(struct NES_Core* nes)
{
    uint32_t cycles = 0;

    while (cycles < CPU_CYCLES_PER_FRAME)
    {
        NES_step(nes);
        cycles += nes->cpu.cycles;
    }
}
