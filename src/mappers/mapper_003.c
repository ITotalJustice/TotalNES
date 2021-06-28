#include "../nes.h"
#include "../internal.h"
#include "mappers.h"

#include <string.h>
#include <assert.h>


#define MAPPER nes->cart.mapper._003

bool mapper_init_003(struct NES_Core* nes, enum Mirror mirror)
{
    memset(&MAPPER, 0, sizeof(MAPPER));

    nes->cart.mapper_type = NesMapperType_003;

    /* prg rom banks */
    MAPPER.prg_rom_slots[0] = nes->cart.pgr_rom;
    MAPPER.prg_rom_slots[1] = nes->cart.pgr_rom_size == 0x4000 ? nes->cart.pgr_rom : nes->cart.pgr_rom + 0x4000;
    
    /* chr rom banks */
    if (nes->cart.chr_rom_size > sizeof(MAPPER.chr_rom))
    {
        NES_log_fatal("invalid chram size!\n");
        return false;
    }
    
    memcpy(MAPPER.chr_rom, nes->cart.chr_rom, nes->cart.chr_rom_size);

    nes->ppu.map[0x0] = MAPPER.chr_rom + 0x0000;
    nes->ppu.map[0x1] = MAPPER.chr_rom + 0x0400;
    nes->ppu.map[0x2] = MAPPER.chr_rom + 0x0800;
    nes->ppu.map[0x3] = MAPPER.chr_rom + 0x0C00;

    nes->ppu.map[0x4] = MAPPER.chr_rom + 0x1000;
    nes->ppu.map[0x5] = MAPPER.chr_rom + 0x1400;
    nes->ppu.map[0x6] = MAPPER.chr_rom + 0x1800;
    nes->ppu.map[0x7] = MAPPER.chr_rom + 0x1C00;

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

uint8_t mapper_read_003(struct NES_Core* nes, uint16_t addr)
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

void mapper_write_003(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
    switch ((addr >> 12) & 0xF)
    {
        case 0x8: case 0x9: case 0xA: case 0xB:
        case 0xC: case 0xD: case 0xE: case 0xF:
            MAPPER.bank_select = value & 0x3;

            nes->ppu.map[0x0] = MAPPER.chr_rom + 0x0000 + (MAPPER.bank_select * 0x2000);
            nes->ppu.map[0x1] = MAPPER.chr_rom + 0x0400 + (MAPPER.bank_select * 0x2000);
            nes->ppu.map[0x2] = MAPPER.chr_rom + 0x0800 + (MAPPER.bank_select * 0x2000);
            nes->ppu.map[0x3] = MAPPER.chr_rom + 0x0C00 + (MAPPER.bank_select * 0x2000);
            
            nes->ppu.map[0x4] = MAPPER.chr_rom + 0x1000 + (MAPPER.bank_select * 0x2000);
            nes->ppu.map[0x5] = MAPPER.chr_rom + 0x1400 + (MAPPER.bank_select * 0x2000);
            nes->ppu.map[0x6] = MAPPER.chr_rom + 0x1800 + (MAPPER.bank_select * 0x2000);
            nes->ppu.map[0x7] = MAPPER.chr_rom + 0x1C00 + (MAPPER.bank_select * 0x2000);
            break;
    }
}

#undef MAPPER
