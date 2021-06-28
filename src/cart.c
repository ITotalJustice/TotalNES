#include "nes.h"
#include "internal.h"
#include "mappers/mappers.h"

#include <string.h>
#include <assert.h>


bool nes_mapper_setup(struct NES_Core* nes, uint8_t mapper, enum Mirror mirror)
{
    switch (mapper)
    {
        case NesMapperType_000: return mapper_init_000(nes, mirror);
        case NesMapperType_001: return mapper_init_001(nes, mirror);
        case NesMapperType_002: return mapper_init_002(nes, mirror);
        case NesMapperType_003: return mapper_init_003(nes, mirror);
    }

    return false;
}

uint8_t nes_cart_read(struct NES_Core* nes, uint16_t addr)
{
    switch (nes->cart.mapper_type)
    {
        case NesMapperType_000: return mapper_read_000(nes, addr);
        case NesMapperType_001: return mapper_read_001(nes, addr);
        case NesMapperType_002: return mapper_read_002(nes, addr);
        case NesMapperType_003: return mapper_read_003(nes, addr);

        default: NES_log_fatal("invalid mapper read\n");
    }

    UNREACHABLE(0xFF);
}

void nes_cart_write(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
    switch (nes->cart.mapper_type)
    {
        case NesMapperType_000: mapper_write_000(nes, addr, value); break;
        case NesMapperType_001: mapper_write_001(nes, addr, value); break;
        case NesMapperType_002: mapper_write_002(nes, addr, value); break;
        case NesMapperType_003: mapper_write_003(nes, addr, value); break;

        default: NES_log_fatal("invalid mapper write\n");
    }
}

