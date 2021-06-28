#include "../nes.h"
#include "../internal.h"

#include <string.h>
#include <assert.h>


#define MAPPER nes->cart.mapper._000


bool mapper_init_000(struct NES_Core* nes, enum Mirror mirror)
{
    memset(&MAPPER, 0, sizeof(MAPPER));

    nes->cart.mapper_type = NesMapperType_000;

    /* prg rom banks */
    MAPPER.prg_rom_slots[0] = nes->cart.pgr_rom;
    MAPPER.prg_rom_slots[1] = nes->cart.pgr_rom_size == 0x4000 ? nes->cart.pgr_rom : nes->cart.pgr_rom + 0x4000;
    
    /* chr rom banks */
    if (nes->cart.chr_rom_size > sizeof(MAPPER.chr_ram))
    {
        NES_log_fatal("invalid chram size!\n");
        return false;
    }
    
    memcpy(MAPPER.chr_ram, nes->cart.chr_rom, nes->cart.chr_rom_size);

    MAPPER.chr_ram_slots[0] = MAPPER.chr_ram + 0x0000;
    MAPPER.chr_ram_slots[1] = MAPPER.chr_ram + 0x1000;

    nes->ppu.map[0x0] = MAPPER.chr_ram_slots[0] + 0x000;
    nes->ppu.map[0x1] = MAPPER.chr_ram_slots[0] + 0x400;
    nes->ppu.map[0x2] = MAPPER.chr_ram_slots[0] + 0x800;
    nes->ppu.map[0x3] = MAPPER.chr_ram_slots[0] + 0xC00;

    nes->ppu.map[0x4] = MAPPER.chr_ram_slots[1] + 0x000;
    nes->ppu.map[0x5] = MAPPER.chr_ram_slots[1] + 0x400;
    nes->ppu.map[0x6] = MAPPER.chr_ram_slots[1] + 0x800;
    nes->ppu.map[0x7] = MAPPER.chr_ram_slots[1] + 0xC00;

    switch (mirror)
    {
        case HORIZONTAL:
            nes->ppu.map[0x8] = nes->ppu.vram + 0x000;
            nes->ppu.map[0x9] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xC] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xD] = nes->ppu.vram + 0x000;

            nes->ppu.map[0xA] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xB] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xE] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xF] = nes->ppu.vram + 0x400;
            break;

        case VERTICAL:
            nes->ppu.map[0x8] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xA] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xC] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xE] = nes->ppu.vram + 0x000;

            nes->ppu.map[0x9] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xB] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xD] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xF] = nes->ppu.vram + 0x400;
            break;

        case FOUR_SCREEN: return false; // unsupported
    }
    return true;
}

uint8_t mapper_read_000(struct NES_Core* nes, uint16_t addr)
{
    switch ((addr >> 12) & 0xF)
    {
        // [CPU]
        case 0x8: case 0x9: case 0xA: case 0xB:
            return MAPPER.prg_rom_slots[0][addr & 0x3FFF];

        case 0xC: case 0xD: case 0xE: case 0xF:
            return MAPPER.prg_rom_slots[1][addr & 0x3FFF];
        
        default:
            return 0xFF;
    }
}

void mapper_write_000(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
    (void)nes; (void)addr; (void)value;
}

#undef MAPPER
