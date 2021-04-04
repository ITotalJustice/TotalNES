#include "core/nes.h"
#include "core/internal.h"

#include <string.h> /* memset */


static const bool MAPPERS[0x100] = {
    [NES_MAPPER_000] = true,
};

static int NES_mapper_init_000(struct NES_Core* nes) {
    memset(&nes->cart.mapper_000, 0, sizeof(nes->cart.mapper_000));
    
    nes->cart.mapper_type = NES_MAPPER_000;

    /* prg rom banks */
    nes->cart.mapper_000.prg_rom_slots[0] = nes->cart.pgr_rom;
    nes->cart.mapper_000.prg_rom_slots[1] = nes->cart.pgr_rom_size == 0x4000 ? nes->cart.pgr_rom : nes->cart.pgr_rom + 0x4000;

    return NES_OK;
}

bool NES_has_mapper(const uint8_t mapper) {
    // checks if the entries are NULL basically.
    return MAPPERS[mapper] == true; 
}

int NES_mapper_setup(struct NES_Core* nes, uint8_t mapper) {
    switch (mapper) {
        case 000: return NES_mapper_init_000(nes);
    }

    return NES_UNSUPORTED_MAPPER;
}

static uint8_t mapper_read_000(struct NES_Core* nes, uint16_t addr) {
    switch ((addr >> 12) & 0xF) {
        case 0x6:
            return nes->cart.mapper_000.prg_ram_slots[0][addr & 0x0FFF];
        case 0x7:
            return nes->cart.mapper_000.prg_ram_slots[1][addr & 0x0FFF];
        case 0x8: case 0x9: case 0xA: case 0xB:
            return nes->cart.mapper_000.prg_rom_slots[0][addr & 0x3FFF];
        case 0xC: case 0xD: case 0xE: case 0xF:
            return nes->cart.mapper_000.prg_rom_slots[1][addr & 0x3FFF];
        default:
            return 0xFF;
    }
}

static void mapper_write_000(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    switch ((addr >> 12) & 0xF) {
        case 0x6:
            nes->cart.mapper_000.prg_ram_slots[0][addr & 0x0FFF]  = value;
            break;
        case 0x7:
            nes->cart.mapper_000.prg_ram_slots[1][addr & 0x0FFF] = value;
            break;
    }
}

/*
static uint8_t mapper_read_001(struct NES_Core* nes, uint16_t addr) {
    NES_UNUSED(nes); NES_UNUSED(addr);
    return 0xFF;
}

static void mapper_write_001(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}

static uint8_t mapper_read_002(struct NES_Core* nes, uint16_t addr) {
    NES_UNUSED(nes); NES_UNUSED(addr);
    return 0xFF;
}

static void mapper_write_002(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}

static uint8_t mapper_read_003(struct NES_Core* nes, uint16_t addr) {
    NES_UNUSED(nes); NES_UNUSED(addr);
    return 0xFF;
}

static void mapper_write_003(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}

static uint8_t mapper_read_004(struct NES_Core* nes, uint16_t addr) {
    NES_UNUSED(nes); NES_UNUSED(addr);
    return 0xFF;
}

static void mapper_write_004(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}
*/

uint8_t NES_cart_read(struct NES_Core* nes, uint16_t addr) {
    switch (nes->cart.mapper_type) {
        case NES_MAPPER_000: return mapper_read_000(nes, addr);

        default:
            assert(0 && "invalid mapper read");
    }

    NES_UNREACHABLE(0xFF);
}

void NES_cart_write(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    switch (nes->cart.mapper_type) {
        case NES_MAPPER_000: mapper_write_000(nes, addr, value); break;

        default:
            assert(0 && "invalid mapper write");
    }
}

