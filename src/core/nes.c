#include "core/nes.h"
#include "core/internal.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>


void NES_reset(struct NES_Core* nes) {
    nes->cpu.A = 0;
    nes->cpu.X = 0;
    nes->cpu.Y = 0;
    nes->cpu.S = 0xFD;
    nes->cpu.PC = 0; // this gets set when the rom is loaded

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
    if (!NES_has_mapper(mapper_num)) {
        NES_log_err("MISSING MAPPER: %u\n", mapper_num);
        return NES_UNSUPORTED_MAPPER;
    }

    return NES_OK;
}

int NES_loadrom(struct NES_Core* nes, uint8_t* buffer, size_t size) {
    assert(nes && buffer && size);

    NES_reset(nes);

    uint8_t* rom = buffer;

    if (!size || size < sizeof(struct NES_CartHeader)) {
        return NES_BAD_ROM;
    }

    const struct NES_CartHeader* header = (struct NES_CartHeader*)rom;

    const uint8_t ines_header = header->header_id[0] == 'N' &&
        header->header_id[1] == 'E' &&
        header->header_id[2] == 'S' &&
        header->header_id[3] == 0x1A;

    if (!ines_header) {
        return NES_UNKNOWN_HEADER;
    }

    /* todo: support ines 2.0  */
    const uint8_t ines_header2 = header->flag7.nes2 == 0x3;
    if (ines_header2) {
        NES_log_err("ines 2.0 header\n");
        return NES_UNKNOWN_HEADER;
    }

    const uint8_t mapper_num = (header->flag7.mapper_num_hi << 4) | header->flag6.mapper_num_lo;

    if (!NES_has_mapper(mapper_num)) {
        NES_log_err("MISSING MAPPER: %u\n", mapper_num);
        return NES_UNSUPORTED_MAPPER;
    }
    NES_log("mapper num: %u\n", mapper_num);

    const uint32_t pgr_rom_size = header->pgr_rom_size * 0x4000;
    const uint32_t chr_rom_size = header->chr_rom_size * 0x2000;

    NES_log("rom size is 0x%llX\n", size);
    NES_log("header size is 0x%llX\n", sizeof(struct NES_CartHeader));
    NES_log("pgr_rom: 0x%X\n", pgr_rom_size);
    NES_log("chr_rom: 0x%X\n", chr_rom_size);

    /* seek past header and trainer (if any). */
    rom += sizeof(struct NES_CartHeader) + (header->flag6.trainer * 0x200);

    NES_log("loaded! trainer size %u\n\n", header->flag6.trainer);

    nes->cart.pgr_rom = rom;
    nes->cart.chr_rom = rom + pgr_rom_size;
    nes->cart.pgr_rom_size = pgr_rom_size;
    nes->cart.chr_rom_size = chr_rom_size;

    NES_mapper_setup(nes, mapper_num);

    // sload from the reset vector
    nes->cpu.PC = NES_cpu_read16(nes, NES_VECTOR_RESET);

    // setup noise lsfr
    nes->apu.noise.lsfr = 0x7FF;

    // i assume all channels start enabled...
    nes->apu.status.square1_enable = 1;
    nes->apu.status.square2_enable = 1;
    nes->apu.status.triangle_enable = 1;
    nes->apu.status.noise_enable = 1;
    nes->apu.status.dmc_enable = 1;

    return NES_OK;
}

void NES_set_apu_callback(struct NES_Core* nes, NES_apu_callback_t cb, void* user_data) {
    nes->apu_cb = cb;
    nes->apu_cb_user_data = user_data;
}

void NES_run_frame(struct NES_Core* nes) {
    assert(nes);

    uint32_t cycles = 0;

    while (cycles < NES_CPU_CYCLES_PER_FRAME) {
        NES_cpu_run(nes);

        // the ppu is 3-times as fast the cpu, so we clock this 3 times.
        for (uint16_t i = 0; i < nes->cpu.cycles; ++i) {
            NES_ppu_run(nes, nes->cpu.cycles);
            NES_ppu_run(nes, nes->cpu.cycles);
            NES_ppu_run(nes, nes->cpu.cycles);
        }

        NES_apu_run(nes, nes->cpu.cycles);

        cycles += nes->cpu.cycles;
    }
}
