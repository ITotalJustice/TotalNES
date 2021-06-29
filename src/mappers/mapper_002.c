#include "../nes.h"
#include "../internal.h"
#include "mappers.h"

#include <string.h>
#include <assert.h>


#define MAPPER nes->cart.mapper._002


enum
{
    MAPPER_002_PRG_RAM_SIZE = 0,
    MAPPER_002_CHR_RAM_SIZE = 1024 * 8,
};


void mapper_get_prg_chr_ram_size_002(size_t* prg_size, size_t* chr_size)
{
    *prg_size = MAPPER_002_PRG_RAM_SIZE;
    *chr_size = MAPPER_002_CHR_RAM_SIZE;
}

bool mapper_init_002(struct NES_Core* nes, enum Mirror mirror)
{
    memset(&MAPPER, 0, sizeof(MAPPER));

    nes->cart.mapper_type = NesMapperType_002;

    /* prg rom banks */
    MAPPER.prg_rom_slots[0] = nes->cart.prg_rom;
    // fixed to the last bank
    MAPPER.prg_rom_slots[1] = nes->cart.prg_rom + (nes->cart.prg_rom_size - PRG_ROM_BANK_SIZE);
    
    mapper_set_pattern_table_bank(nes, 0, 0x0000);
    mapper_set_pattern_table_bank(nes, 1, 0x1000);
    mapper_set_nametable_mirroring(nes, mirror);

    return true;
}

uint8_t mapper_read_002(struct NES_Core* nes, uint16_t addr)
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

void mapper_write_002(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
    switch ((addr >> 12) & 0xF)
    {
        case 0x8: case 0x9: case 0xA: case 0xB:
        case 0xC: case 0xD: case 0xE: case 0xF:
            MAPPER.bank_select = value & 0xF;
            MAPPER.prg_rom_slots[0] = nes->cart.prg_rom + (MAPPER.bank_select * PRG_ROM_BANK_SIZE);
            break;
    }
}

#undef MAPPER
