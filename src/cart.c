#include "nes.h"
#include "internal.h"
#include "mappers/mappers.h"

#include <string.h>
#include <assert.h>


bool nes_mapper_get_prg_chr_ram_size(uint8_t mapper, size_t* prg_size, size_t* chr_size)
{
    switch (mapper)
    {
        case NesMapperType_000: mapper_get_prg_chr_ram_size_000(prg_size, chr_size); return true;
        case NesMapperType_001: mapper_get_prg_chr_ram_size_001(prg_size, chr_size); return true;
        case NesMapperType_002: mapper_get_prg_chr_ram_size_002(prg_size, chr_size); return true;
        case NesMapperType_003: mapper_get_prg_chr_ram_size_003(prg_size, chr_size); return true;
        case NesMapperType_007: mapper_get_prg_chr_ram_size_007(prg_size, chr_size); return true;
    }

    return false;
}

bool nes_mapper_setup(struct NES_Core* nes, uint8_t mapper, enum Mirror mirror)
{
    switch (mapper)
    {
        case NesMapperType_000: return mapper_init_000(nes, mirror);
        case NesMapperType_001: return mapper_init_001(nes, mirror);
        case NesMapperType_002: return mapper_init_002(nes, mirror);
        case NesMapperType_003: return mapper_init_003(nes, mirror);
        case NesMapperType_007: return mapper_init_007(nes, mirror);
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
        case NesMapperType_007: return mapper_read_007(nes, addr);

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
        case NesMapperType_007: mapper_write_007(nes, addr, value); break;

        default: NES_log_fatal("invalid mapper write\n");
    }
}

void mapper_set_pattern_table(struct NES_Core* nes, uint8_t table, const uint8_t* rptr, uint8_t* wptr)
{
    assert(table <= 1);

    switch (table & 0x1)
    {
        case 0x0:
            nes->ppu.read_map[0x0] = rptr + 0x000;
            nes->ppu.read_map[0x1] = rptr + 0x400;
            nes->ppu.read_map[0x2] = rptr + 0x800;
            nes->ppu.read_map[0x3] = rptr + 0xC00;

            nes->ppu.write_map[0x0] = wptr + 0x000;
            nes->ppu.write_map[0x1] = wptr + 0x400;
            nes->ppu.write_map[0x2] = wptr + 0x800;
            nes->ppu.write_map[0x3] = wptr + 0xC00;
            break;

        case 0x1:
            nes->ppu.read_map[0x4] = rptr + 0x000;
            nes->ppu.read_map[0x5] = rptr + 0x400;
            nes->ppu.read_map[0x6] = rptr + 0x800;
            nes->ppu.read_map[0x7] = rptr + 0xC00;

            nes->ppu.write_map[0x4] = wptr + 0x000;
            nes->ppu.write_map[0x5] = wptr + 0x400;
            nes->ppu.write_map[0x6] = wptr + 0x800;
            nes->ppu.write_map[0x7] = wptr + 0xC00;
            break;
    }
}

void mapper_set_nametable(struct NES_Core* nes, uint8_t table, const uint8_t* rptr, uint8_t* wptr)
{
    assert(table <= 3);

    switch (table & 0x3)
    {
        case 0x0:
            nes->ppu.read_map[0x8] = rptr;
            nes->ppu.read_map[0xC] = rptr;
            nes->ppu.write_map[0x8] = wptr;
            nes->ppu.write_map[0xC] = wptr;
            break;

        case 0x1:
            nes->ppu.read_map[0x9] = rptr;
            nes->ppu.read_map[0xD] = rptr;
            nes->ppu.write_map[0x9] = wptr;
            nes->ppu.write_map[0xD] = wptr;
            break;

        case 0x2:
            nes->ppu.read_map[0xA] = rptr;
            nes->ppu.read_map[0xE] = rptr;
            nes->ppu.write_map[0xA] = wptr;
            nes->ppu.write_map[0xE] = wptr;
            break;

        case 0x3:
            nes->ppu.read_map[0xB] = rptr;
            nes->ppu.read_map[0xF] = rptr;
            nes->ppu.write_map[0xB] = wptr;
            nes->ppu.write_map[0xF] = wptr;
            break;
    }
}

void mapper_set_pattern_table_bank(struct NES_Core* nes, uint8_t table, uint32_t offset)
{
    assert(table <= 1);

    if (nes->cart.chr_rom_size)
    {
        const uint8_t* ptr = nes->cart.chr_rom + offset;

        mapper_set_pattern_table(nes, table, ptr, NULL);
    }
    else
    {
        uint8_t* ptr = nes->chr_ram + offset;
        
        mapper_set_pattern_table(nes, table, ptr, ptr);
    }
}

void mapper_set_nametable_mirroring(struct NES_Core* nes, enum Mirror mirror)
{
    switch (mirror)
    {
        // one-screen lower bank
        case ONE_SCREEN_LOW:
            mapper_set_nametable(nes, 0, nes->ppu.vram + 0x000, nes->ppu.vram + 0x000);
            mapper_set_nametable(nes, 1, nes->ppu.vram + 0x000, nes->ppu.vram + 0x000);
            mapper_set_nametable(nes, 2, nes->ppu.vram + 0x000, nes->ppu.vram + 0x000);
            mapper_set_nametable(nes, 3, nes->ppu.vram + 0x000, nes->ppu.vram + 0x000);
            break;

        // one-screen upper bank
        case ONE_SCREEN_HIGH:
            mapper_set_nametable(nes, 0, nes->ppu.vram + 0x400, nes->ppu.vram + 0x400);
            mapper_set_nametable(nes, 1, nes->ppu.vram + 0x400, nes->ppu.vram + 0x400);
            mapper_set_nametable(nes, 2, nes->ppu.vram + 0x400, nes->ppu.vram + 0x400);
            mapper_set_nametable(nes, 3, nes->ppu.vram + 0x400, nes->ppu.vram + 0x400);
            break;

        // vertical
        case VERTICAL:
            mapper_set_nametable(nes, 0, nes->ppu.vram + 0x000, nes->ppu.vram + 0x000);
            mapper_set_nametable(nes, 1, nes->ppu.vram + 0x400, nes->ppu.vram + 0x400);
            mapper_set_nametable(nes, 2, nes->ppu.vram + 0x000, nes->ppu.vram + 0x000);
            mapper_set_nametable(nes, 3, nes->ppu.vram + 0x400, nes->ppu.vram + 0x400);
            break;

        // horizontal
        case HORIZONTAL:
            mapper_set_nametable(nes, 0, nes->ppu.vram + 0x000, nes->ppu.vram + 0x000);
            mapper_set_nametable(nes, 1, nes->ppu.vram + 0x000, nes->ppu.vram + 0x000);
            mapper_set_nametable(nes, 2, nes->ppu.vram + 0x400, nes->ppu.vram + 0x400);
            mapper_set_nametable(nes, 3, nes->ppu.vram + 0x400, nes->ppu.vram + 0x400);
            break;

        case FOUR_SCREEN:
            NES_log_fatal("FOUR_SCREEN mapper not impl!\n");
            break;
    } 
}
