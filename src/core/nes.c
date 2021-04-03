#include "core/nes.h"
#include "core/internal.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>


#define JUMP_ERR(label,err,code) err = code; goto label;


void NES_reset(struct NES_Core* nes) {
    nes->cpu.A = 0;
    nes->cpu.X = 0;
    nes->cpu.Y = 0;
    nes->cpu.S = 0xFD;
    /* for nestest */
    nes->cpu.PC = 0xC000;

    nes->cpu.status.I = 1;
    nes->cpu.status.D = 0;
    nes->cpu.status.B = 2;
}

int NES_is_header_valid(struct NES_Core* nes, const struct NES_CartHeader* header) {
    assert(nes && header);
    
    const uint8_t ines_header =
        header->header_id[0] == 'N' &&
        header->header_id[1] == 'E' &&
        header->header_id[2] == 'S' &&
        header->header_id[3] == 0x1A;
    
    if (!ines_header) {
        return NES_UNKNOWN_HEADER;
    }

    const uint8_t ines_header2 = header->flag7.nes2 == 0x3;
    if (ines_header2) {
        NES_log_err("ines 2.0 header\n");
        return NES_UNKNOWN_HEADER;
    }

    const uint8_t mapper_num = (header->flag7.mapper_num_hi << 4) | header->flag6.mapper_num_lo;
    if (NES_OK != NES_has_mapper(nes, mapper_num)) {
        NES_log_err("MISSING MAPPER: %u\n", mapper_num);
        return NES_UNSUPORTED_MAPPER;
    }

    return NES_OK;
}

int NES_loadrom(struct NES_Core* nes, uint8_t* buffer, size_t size) {
    assert(nes && buffer && size);

    NES_reset(nes);

    uint8_t* rom = buffer;
    int err_code = NES_OK;

    if (!size || size < sizeof(struct NES_CartHeader)) {
        JUMP_ERR(err, err_code, NES_BAD_ROM);
    }

    const struct NES_CartHeader* header = (struct NES_CartHeader*)rom;

    const uint8_t ines_header = header->header_id[0] == 'N' &&
        header->header_id[1] == 'E' &&
        header->header_id[2] == 'S' &&
        header->header_id[3] == 0x1A;
    
    if (!ines_header) {
        JUMP_ERR(err, err_code, NES_UNKNOWN_HEADER);
    }

    /* todo: support ines 2.0  */
    const uint8_t ines_header2 = header->flag7.nes2 == 0x3;
    if (ines_header2) {
        NES_log_err("ines 2.0 header\n");
        JUMP_ERR(err, err_code, NES_UNKNOWN_HEADER);
    }
    
    const uint8_t mapper_num = (header->flag7.mapper_num_hi << 4) | header->flag6.mapper_num_lo;
    if (NES_OK != NES_has_mapper(nes, mapper_num)) {
        NES_log_err("MISSING MAPPER: %u\n", mapper_num);
        JUMP_ERR(err, err_code, NES_UNSUPORTED_MAPPER);
    }
    NES_log("mapper num: %u\n", mapper_num);

    const uint32_t pgr_rom_size = header->pgr_rom_size * 0x4000;
    const uint32_t chr_rom_size = header->chr_rom_size * 0x2000;
    NES_log("pgr_rom: 0x%X\n", pgr_rom_size);
    NES_log("chr_rom: 0x%X\n", chr_rom_size);

    /* seek past header and trainer (if any). */
    rom += sizeof(struct NES_CartHeader) + (header->flag6.trainer * 0x200);

    NES_log("loaded!\n\n");
    nes->cart.pgr_rom = rom;
    nes->cart.chr_rom = rom + pgr_rom_size;
    nes->cart.pgr_rom_size = pgr_rom_size;
    nes->cart.chr_rom_size = chr_rom_size;

    NES_mapper_setup(nes, mapper_num);

    return err_code;

    err:
    return err_code;
}

#define CPU_MAX_CYCLES (1789773/60)

int NES_run_frame(struct NES_Core* nes) {
    assert(nes);
    uint32_t cycles = 0;

    do {
        nes->cpu.cycles = 0;
        NES_cpu_run(nes);
        cycles += nes->cpu.cycles;
        NES_ppu_run(nes);
    } while (cycles < CPU_MAX_CYCLES);

    return NES_OK;
}
