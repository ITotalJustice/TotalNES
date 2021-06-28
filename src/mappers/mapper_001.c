#include "../nes.h"
#include "../internal.h"
#include "mappers.h"

#include <string.h>
#include <assert.h>


#define MAPPER nes->cart.mapper._001


static void mapper_update_chr_bank0_001(struct NES_Core* nes)
{
    if (MAPPER.chr_rom_bank_mode)
    {
        // switch 4KiB
        MAPPER.chr_ram_slots[0] = MAPPER.chr_ram + (MAPPER.chr_bank0 * 0x1000);

        nes->ppu.map[0x0] = MAPPER.chr_ram_slots[0] + 0x000;
        nes->ppu.map[0x1] = MAPPER.chr_ram_slots[0] + 0x400;
        nes->ppu.map[0x2] = MAPPER.chr_ram_slots[0] + 0x800;
        nes->ppu.map[0x3] = MAPPER.chr_ram_slots[0] + 0xC00;
    }
    else
    {
        // switch 8KiB
        MAPPER.chr_ram_slots[0] = MAPPER.chr_ram + ((MAPPER.chr_bank0 & ~ 0x1) * 0x2000);
        MAPPER.chr_ram_slots[1] = MAPPER.chr_ram_slots[0] + 0x1000;

        nes->ppu.map[0x0] = MAPPER.chr_ram_slots[0] + 0x000;
        nes->ppu.map[0x1] = MAPPER.chr_ram_slots[0] + 0x400;
        nes->ppu.map[0x2] = MAPPER.chr_ram_slots[0] + 0x800;
        nes->ppu.map[0x3] = MAPPER.chr_ram_slots[0] + 0xC00;

        nes->ppu.map[0x4] = MAPPER.chr_ram_slots[1] + 0x000;
        nes->ppu.map[0x5] = MAPPER.chr_ram_slots[1] + 0x400;
        nes->ppu.map[0x6] = MAPPER.chr_ram_slots[1] + 0x800;
        nes->ppu.map[0x7] = MAPPER.chr_ram_slots[1] + 0xC00;
    }
}

static void mapper_update_chr_bank1_001(struct NES_Core* nes)
{
    // this is ignored in 8kb mode
    if (MAPPER.chr_rom_bank_mode)
    {
        MAPPER.chr_ram_slots[1] = MAPPER.chr_ram + (MAPPER.chr_bank1 * 0x1000);

        nes->ppu.map[0x4] = MAPPER.chr_ram_slots[1] + 0x000;
        nes->ppu.map[0x5] = MAPPER.chr_ram_slots[1] + 0x400;
        nes->ppu.map[0x6] = MAPPER.chr_ram_slots[1] + 0x800;
        nes->ppu.map[0x7] = MAPPER.chr_ram_slots[1] + 0xC00;
    }
}

static void mapper_update_prg_bank_001(struct NES_Core* nes)
{
    switch (MAPPER.prg_rom_bank_mode)
    {
        case 0x0: case 0x1:
            // in 32K mode, bit0 is ignored of bank select
            MAPPER.prg_rom_slots[0] = nes->cart.pgr_rom + ((MAPPER.prg_bank & ~1) * (1024 * 32));
            MAPPER.prg_rom_slots[1] = MAPPER.prg_rom_slots[0] + PRG_ROM_BANK_SIZE;
            break;

        case 0x2:
            MAPPER.prg_rom_slots[0] = nes->cart.pgr_rom;
            MAPPER.prg_rom_slots[1] = nes->cart.pgr_rom + (MAPPER.prg_bank * PRG_ROM_BANK_SIZE);
            break;

        case 0x3:
            MAPPER.prg_rom_slots[0] = nes->cart.pgr_rom + (MAPPER.prg_bank * PRG_ROM_BANK_SIZE);
            MAPPER.prg_rom_slots[1] = nes->cart.pgr_rom + (nes->cart.pgr_rom_size - PRG_ROM_BANK_SIZE);
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
        // one-screen lower bank
        case 0x0:
            nes->ppu.map[0x8] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xA] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xC] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xE] = nes->ppu.vram + 0x000;

            nes->ppu.map[0x9] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xB] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xD] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xF] = nes->ppu.vram + 0x000;
            break;

        // one-screen upper bank
        case 0x1:
            nes->ppu.map[0x8] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xA] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xC] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xE] = nes->ppu.vram + 0x400;

            nes->ppu.map[0x9] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xB] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xD] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xF] = nes->ppu.vram + 0x400;
            break;

        // vertical
        case 0x2:
            nes->ppu.map[0x8] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xA] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xC] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xE] = nes->ppu.vram + 0x000;

            nes->ppu.map[0x9] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xB] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xD] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xF] = nes->ppu.vram + 0x400;
            break;

        // horizontal
        case 0x3:
            nes->ppu.map[0x8] = nes->ppu.vram + 0x000;
            nes->ppu.map[0x9] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xC] = nes->ppu.vram + 0x000;
            nes->ppu.map[0xD] = nes->ppu.vram + 0x000;

            nes->ppu.map[0xA] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xB] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xE] = nes->ppu.vram + 0x400;
            nes->ppu.map[0xF] = nes->ppu.vram + 0x400;
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
    MAPPER.prg_rom_slots[0] = nes->cart.pgr_rom;
    MAPPER.prg_rom_slots[1] = nes->cart.pgr_rom_size == 0x4000 ? nes->cart.pgr_rom : nes->cart.pgr_rom + 0x4000;
    
    /* chr rom banks */
    if (nes->cart.chr_rom_size > sizeof(MAPPER.chr_ram))
    {
        NES_log_fatal("invalid chram size!\n");
        return false;
    }
    
    memcpy(MAPPER.chr_ram, nes->cart.chr_rom, nes->cart.chr_rom_size);

    mapper_write_control_001(nes, 0x1C);

    return true;
}

uint8_t mapper_read_001(struct NES_Core* nes, uint16_t addr)
{
    switch ((addr >> 12) & 0xF)
    {
        case 0x6: case 0x7:
            if (MAPPER.prg_ram_enable)
            {
                return MAPPER.prg_ram[addr & 0x1FFF];
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
            MAPPER.prg_ram[addr & 0x1FFF] = value;
        }
    }
    // if bit7 is set, shift reg is reset
    else if (value & 0x80)
    {
        MAPPER.shift_reg = 0;
        MAPPER.shift_reg_count = 0;
        mapper_write_control_001(nes, 0x1C);
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
