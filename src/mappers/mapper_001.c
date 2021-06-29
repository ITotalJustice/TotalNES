#include "../nes.h"
#include "../internal.h"
#include "mappers.h"

#include <string.h>
#include <assert.h>


#define MAPPER nes->cart.mapper._001


enum
{
    MAPPER_001_PRG_RAM_SIZE = 1024 * 8,
    MAPPER_001_CHR_RAM_SIZE = 1024 * 8,
};


void mapper_get_prg_chr_ram_size_001(size_t* prg_size, size_t* chr_size)
{
    *prg_size = MAPPER_001_PRG_RAM_SIZE;
    *chr_size = MAPPER_001_CHR_RAM_SIZE;
}

static void mapper_update_chr_bank0_001(struct NES_Core* nes)
{
    if (MAPPER.chr_rom_bank_mode)
    {
        // switch 4KiB
        const uint32_t bank_offset = MAPPER.chr_bank0 * 0x1000;

        mapper_set_pattern_table_bank(nes, 0, bank_offset);
    }
    else
    {
        // switch 8KiB
        const uint32_t bank_offset = (MAPPER.chr_bank0 & ~ 0x1) * 0x2000;

        mapper_set_pattern_table_bank(nes, 0, bank_offset + 0x0000);
        mapper_set_pattern_table_bank(nes, 1, bank_offset + 0x1000);
    }
}

static void mapper_update_chr_bank1_001(struct NES_Core* nes)
{
    // this is ignored in 8kb mode
    if (MAPPER.chr_rom_bank_mode)
    {
        const uint32_t bank_offset = MAPPER.chr_bank1 * 0x1000;

        mapper_set_pattern_table_bank(nes, 1, bank_offset);
    }
}

static void mapper_update_prg_bank_001(struct NES_Core* nes)
{
    switch (MAPPER.prg_rom_bank_mode)
    {
        case 0x0: case 0x1:
            // in 32K mode, bit0 is ignored of bank select
            MAPPER.prg_rom_slots[0] = nes->cart.prg_rom + ((MAPPER.prg_bank & ~1) * (1024 * 32));
            MAPPER.prg_rom_slots[1] = MAPPER.prg_rom_slots[0] + PRG_ROM_BANK_SIZE;
            break;

        case 0x2:
            MAPPER.prg_rom_slots[0] = nes->cart.prg_rom;
            MAPPER.prg_rom_slots[1] = nes->cart.prg_rom + (MAPPER.prg_bank * PRG_ROM_BANK_SIZE);
            break;

        case 0x3:
            MAPPER.prg_rom_slots[0] = nes->cart.prg_rom + (MAPPER.prg_bank * PRG_ROM_BANK_SIZE);
            MAPPER.prg_rom_slots[1] = nes->cart.prg_rom + (nes->cart.prg_rom_size - PRG_ROM_BANK_SIZE);
            break;
    }

    // todo: update ram
}

static void mapper_write_control_001(struct NES_Core* nes, uint8_t value)
{
    MAPPER.mirroring = (value >> 0) & 0x3;
    MAPPER.prg_rom_bank_mode = (value >> 2) & 0x3;
    MAPPER.chr_rom_bank_mode = (value >> 4) & 0x1;

    switch (MAPPER.mirroring)
    {
        case 0x0:
            mapper_set_nametable_mirroring(nes, ONE_SCREEN_LOW);
            break;

        case 0x1:
            mapper_set_nametable_mirroring(nes, ONE_SCREEN_HIGH);
            break;

        case 0x2:
            mapper_set_nametable_mirroring(nes, VERTICAL);
            break;

        case 0x3:
            mapper_set_nametable_mirroring(nes, HORIZONTAL);
            break;
    }

    mapper_update_chr_bank0_001(nes);
    mapper_update_chr_bank1_001(nes);
    mapper_update_prg_bank_001(nes);
}

static void mapper_write_chr_bank0_001(struct NES_Core* nes, uint8_t value)
{
    MAPPER.chr_bank0 = value & 0x1F;

    mapper_update_chr_bank0_001(nes);
}

static void mapper_write_chr_bank1_001(struct NES_Core* nes, uint8_t value)
{
    MAPPER.chr_bank1 = value & 0x1F;

    mapper_update_chr_bank1_001(nes);
}

static void mapper_write_prg_bank_001(struct NES_Core* nes, uint8_t value)
{
    MAPPER.prg_bank = (value >> 0) & 0xF;
    // invert because 0=enabled!
    MAPPER.prg_ram_enable = !((value >> 4) & 0x1);

    mapper_update_prg_bank_001(nes);
}

bool mapper_init_001(struct NES_Core* nes, enum Mirror mirror)
{
    (void)mirror;
    memset(&MAPPER, 0, sizeof(MAPPER));

    nes->cart.mapper_type = NesMapperType_001;

    /* prg rom banks */
    MAPPER.prg_rom_slots[0] = nes->cart.prg_rom;
    MAPPER.prg_rom_slots[1] = nes->cart.prg_rom_size == 0x4000 ? nes->cart.prg_rom : nes->cart.prg_rom + 0x4000;
    
    if (nes->prg_ram_size < MAPPER_001_PRG_RAM_SIZE)
    {
        NES_log_fatal("invalid prg ram size!\n");
        return false;
    }

    if (!nes->cart.chr_rom_size && nes->chr_ram_size < MAPPER_001_CHR_RAM_SIZE)
    {
        NES_log_fatal("invalid chr ram size!\n");
        return false;
    }
    
    mapper_write_control_001(nes, 0x0C);

    return true;
}

uint8_t mapper_read_001(struct NES_Core* nes, uint16_t addr)
{
    switch ((addr >> 12) & 0xF)
    {
        case 0x6: case 0x7:
            if (MAPPER.prg_ram_enable)
            {
                return nes->prg_ram[addr & 0x1FFF];
            }
            else
            {
                return 0xFF;
            }

        case 0x8: case 0x9: case 0xA: case 0xB:
            return MAPPER.prg_rom_slots[0][addr & 0x3FFF];

        case 0xC: case 0xD: case 0xE: case 0xF:
            return MAPPER.prg_rom_slots[1][addr & 0x3FFF];
        
        default:
            return 0xFF;
    }
}

void mapper_write_001(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
    // handle ram writes here
    if (addr >= 0x6000 && addr <= 0x7FFF)
    {
        if (MAPPER.prg_ram_enable)
        {
            nes->prg_ram[addr & 0x1FFF] = value;
        }
    }
    // if bit7 is set, shift reg is reset
    else if (value & 0x80)
    {
        MAPPER.shift_reg = 0;
        MAPPER.shift_reg_count = 0;
        mapper_write_control_001(nes, 0x0C);
    }
    else
    {
        // LSB first!
        MAPPER.shift_reg >>= 1;
        MAPPER.shift_reg |= (value & 0x1) << 4;
        MAPPER.shift_reg_count++;

        // on the fifth write do we actually do something!
        if (MAPPER.shift_reg_count == 5)
        {
            switch ((addr >> 12) & 0xF)
            {
                // control
                case 0x8: case 0x9:
                    mapper_write_control_001(nes, MAPPER.shift_reg);
                    break;

                // chr bank 0
                case 0xA: case 0xB:
                    mapper_write_chr_bank0_001(nes, MAPPER.shift_reg);
                    break;

                // chr bank 1
                case 0xC: case 0xD:
                    mapper_write_chr_bank1_001(nes, MAPPER.shift_reg);
                    break;

                // prg bank
                case 0xE: case 0xF:
                    mapper_write_prg_bank_001(nes, MAPPER.shift_reg);
                    break;
            }

            // finally, these are reset.
            MAPPER.shift_reg = 0;
            MAPPER.shift_reg_count = 0;
        }
    }
}

#undef MAPPER
