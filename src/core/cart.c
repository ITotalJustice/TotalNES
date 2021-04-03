#include "core/nes.h"
#include "core/internal.h"

#include <string.h> /* memset */


static uint8_t NES_mapper_read_000(struct NES_Core* nes, uint16_t addr);
static void NES_mapper_write_000(struct NES_Core* nes, uint16_t addr, uint8_t value);

static uint8_t NES_mapper_read_001(struct NES_Core* nes, uint16_t addr);
static void NES_mapper_write_001(struct NES_Core* nes, uint16_t addr, uint8_t value);

static uint8_t NES_mapper_read_002(struct NES_Core* nes, uint16_t addr);
static void NES_mapper_write_002(struct NES_Core* nes, uint16_t addr, uint8_t value);

static uint8_t NES_mapper_read_003(struct NES_Core* nes, uint16_t addr);
static void NES_mapper_write_003(struct NES_Core* nes, uint16_t addr, uint8_t value);

static uint8_t NES_mapper_read_004(struct NES_Core* nes, uint16_t addr);
static void NES_mapper_write_004(struct NES_Core* nes, uint16_t addr, uint8_t value);

static uint8_t NES_mapper_read_005(struct NES_Core* nes, uint16_t addr);
static void NES_mapper_write_005(struct NES_Core* nes, uint16_t addr, uint8_t value);

static uint8_t NES_mapper_read_009(struct NES_Core* nes, uint16_t addr);
static void NES_mapper_write_009(struct NES_Core* nes, uint16_t addr, uint8_t value);

static uint8_t NES_mapper_read_010(struct NES_Core* nes, uint16_t addr);
static void NES_mapper_write_010(struct NES_Core* nes, uint16_t addr, uint8_t value);

static uint8_t NES_mapper_read_037(struct NES_Core* nes, uint16_t addr);
static void NES_mapper_write_037(struct NES_Core* nes, uint16_t addr, uint8_t value);

static uint8_t NES_mapper_read_047(struct NES_Core* nes, uint16_t addr);
static void NES_mapper_write_047(struct NES_Core* nes, uint16_t addr, uint8_t value);

struct NES_MapperEntry {
    uint8_t (*mapper_read)(struct NES_Core*, uint16_t);
    void (*mapper_write)(struct NES_Core*, uint16_t, uint8_t);
};

static const struct NES_MapperEntry MAPPERS[0x100] = {
    [000] = { NES_mapper_read_000, NES_mapper_write_000 },
    // [001] = { NES_mapper_read_001, NES_mapper_write_001 },
    // [002] = { NES_mapper_read_002, NES_mapper_write_002 },
    // [003] = { NES_mapper_read_003, NES_mapper_write_003 },
    // [004] = { NES_mapper_read_004, NES_mapper_write_004 },
    // [005] = { NES_mapper_read_005, NES_mapper_write_005 },
    // [009] = { NES_mapper_read_009, NES_mapper_write_009 },
    // [010] = { NES_mapper_read_010, NES_mapper_write_010 },
    // [037] = { NES_mapper_read_037, NES_mapper_write_037 },
    // [047] = { NES_mapper_read_047, NES_mapper_write_047 },
    // [066] = { NES_mapper_read_066, NES_mapper_write_066 },
    // [099] = { NES_mapper_read_099, NES_mapper_write_099 },
    // [105] = { NES_mapper_read_105, NES_mapper_write_105 },
};

static int NES_mapper_init_000(struct NES_Core* nes) {
    memset(&nes->cart.mapper_000, 0, sizeof(nes->cart.mapper_000));
    nes->cart.mapper_read = MAPPERS[0].mapper_read;
    nes->cart.mapper_write = MAPPERS[0].mapper_write;
    /* prg rom banks */
    nes->cart.mapper_000.prg_rom_slots[0] = nes->cart.pgr_rom;
    nes->cart.mapper_000.prg_rom_slots[1] = nes->cart.pgr_rom_size == 0x4000 ? nes->cart.pgr_rom : nes->cart.pgr_rom + 0x4000;
    return NES_OK;
}

bool NES_has_mapper(struct NES_Core* nes, uint8_t mapper) {
    // checks if the entries are NULL basically.
    return MAPPERS[mapper].mapper_read && MAPPERS[mapper].mapper_write; 
}

int NES_mapper_setup(struct NES_Core* nes, uint8_t mapper) {
    switch (mapper) {
        case 000: return NES_mapper_init_000(nes);
    }

    return NES_UNSUPORTED_MAPPER;
}

static uint8_t NES_mapper_read_000(struct NES_Core* nes, uint16_t addr) {
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

static void NES_mapper_write_000(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    switch ((addr >> 12) & 0xF) {
        case 0x6:
            nes->cart.mapper_000.prg_ram_slots[0][addr & 0x0FFF]  = value;
            break;
        case 0x7:
            nes->cart.mapper_000.prg_ram_slots[1][addr & 0x0FFF] = value;
            break;
    }
}

static uint8_t NES_mapper_read_001(struct NES_Core* nes, uint16_t addr) {
    NES_UNUSED(nes); NES_UNUSED(addr);
    return 0xFF;
}

static void NES_mapper_write_001(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}

static uint8_t NES_mapper_read_002(struct NES_Core* nes, uint16_t addr) {
    NES_UNUSED(nes); NES_UNUSED(addr);
    return 0xFF;
}

static void NES_mapper_write_002(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}

static uint8_t NES_mapper_read_003(struct NES_Core* nes, uint16_t addr) {
    NES_UNUSED(nes); NES_UNUSED(addr);
    return 0xFF;
}

static void NES_mapper_write_003(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}

static uint8_t NES_mapper_read_004(struct NES_Core* nes, uint16_t addr) {
    NES_UNUSED(nes); NES_UNUSED(addr);
    return 0xFF;
}

static void NES_mapper_write_004(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    NES_UNUSED(nes); NES_UNUSED(addr); NES_UNUSED(value);
}
