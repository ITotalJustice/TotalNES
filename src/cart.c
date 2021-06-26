#include "nes.h"
#include "internal.h"

#include <string.h> /* memset */
#include <stdio.h>
#include <assert.h>


static const bool MAPPERS[0x100] =
{
    [NES_MAPPER_000] = true,
};

static bool nes_mapper_init_000(struct NES_Core* nes)
{
    memset(&nes->cart.mapper._000, 0, sizeof(nes->cart.mapper._000));

    nes->cart.mapper_type = NES_MAPPER_000;

    /* prg rom banks */
    nes->cart.mapper._000.prg_rom_slots[0] = nes->cart.pgr_rom;
    nes->cart.mapper._000.prg_rom_slots[1] = nes->cart.pgr_rom_size == 0x4000 ? nes->cart.pgr_rom : nes->cart.pgr_rom + 0x4000;
    /* chr rom banks */
    assert(sizeof(nes->cart.mapper._000.chr_ram) >= nes->cart.chr_rom_size);
    memcpy(nes->cart.mapper._000.chr_ram, nes->cart.chr_rom, nes->cart.chr_rom_size);

    nes->cart.mapper._000.chr_ram_slots[0] = nes->cart.mapper._000.chr_ram;
    nes->cart.mapper._000.chr_ram_slots[1] = nes->cart.mapper._000.chr_ram + 0x1000;

    return true;
}

bool nes_has_mapper(const uint8_t mapper)
{
    // checks if the entries are NULL basically.
    return MAPPERS[mapper] == true; 
}

bool nes_mapper_setup(struct NES_Core* nes, uint8_t mapper)
{
    switch (mapper)
    {
        case 000: return nes_mapper_init_000(nes);
    }

    return false;
}

static inline uint8_t mapper_read_000(struct NES_Core* nes, uint16_t addr)
{
    switch ((addr >> 12) & 0xF)
    {
        // [PPU]
        case 0x0:
            return nes->cart.mapper._000.chr_ram_slots[0][addr & 0x0FFF];
        
        case 0x1:
            return nes->cart.mapper._000.chr_ram_slots[1][addr & 0x0FFF];

        // [CPU]
        case 0x8: case 0x9: case 0xA: case 0xB:
            return nes->cart.mapper._000.prg_rom_slots[0][addr & 0x3FFF];

        case 0xC: case 0xD: case 0xE: case 0xF:
            return nes->cart.mapper._000.prg_rom_slots[1][addr & 0x3FFF];
        
        default:
            return 0xFF;
    }
}

static inline void mapper_write_000(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
    switch ((addr >> 12) & 0xF)
    {
        // [PPU]
        case 0x0:
            nes->cart.mapper._000.chr_ram_slots[0][addr & 0x0FFF] = value;
            break;

        case 0x1:
            nes->cart.mapper._000.chr_ram_slots[1][addr & 0x0FFF] = value;
            break;
    }
}

/*
static inline uint8_t mapper_read_001(struct NES_Core* nes, uint16_t addr)
{
  NES_UNUSED(nes); NES_UNUSED(addr);
  return 0xFF;
}

static inline void mapper_write_001(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
  NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}

static inline uint8_t mapper_read_002(struct NES_Core* nes, uint16_t addr)
{
  NES_UNUSED(nes); NES_UNUSED(addr);
  return 0xFF;
}

static inline void mapper_write_002(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
  NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}

static inline uint8_t mapper_read_003(struct NES_Core* nes, uint16_t addr)
{
  NES_UNUSED(nes); NES_UNUSED(addr);
  return 0xFF;
}

static inline void mapper_write_003(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
  NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}

static inline uint8_t mapper_read_004(struct NES_Core* nes, uint16_t addr)
{
  NES_UNUSED(nes); NES_UNUSED(addr);
  return 0xFF;
}

static inline void mapper_write_004(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
  NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}
*/

uint8_t nes_cart_read(struct NES_Core* nes, uint16_t addr)
{
    switch (nes->cart.mapper_type)
    {
        case NES_MAPPER_000: return mapper_read_000(nes, addr);

        default: NES_log_fatal("invalid mapper read\n");
    }

    UNREACHABLE(0xFF);
}

void nes_cart_write(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
    switch (nes->cart.mapper_type)
    {
        case NES_MAPPER_000: mapper_write_000(nes, addr, value); break;

        default: NES_log_fatal("invalid mapper write\n");
    }
}

