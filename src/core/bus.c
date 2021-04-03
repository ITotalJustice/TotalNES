#include "core/nes.h"
#include "core/internal.h"

#include <assert.h>


static inline uint8_t NES_cpu_io_read(struct NES_Core* nes, uint8_t addr) {
    uint8_t data;

    switch (addr & 0x1F) {
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
        case 0x10: case 0x11: case 0x12: case 0x13:
            data = nes->apu.io[addr];
            break;
        case 0x14:
            data = nes->ppu.oam_addr;
            break;
        case 0x15:
            data = nes->apu.io[addr];
            nes->apu.status.frame_irq = 0;
            break;
        case 0x16: /* controller 1 */
            data = 0xFF;
            break;
        case 0x17: /* controller 2 */
            data = 0xFF;
            break;

        default: NES_UNREACHABLE(0xFF);
    }

    return data;
}

static inline void NES_cpu_io_write(struct NES_Core* nes, uint8_t addr, uint8_t value) {
    switch (addr & 0x1F) {
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
        case 0x10: case 0x11: case 0x12: case 0x13:
            nes->apu.io[addr] = value;
            break;
        case 0x14:
            nes->ppu.oam_addr = value;
            NES_dma(nes);
            break;
        case 0x15:
            nes->apu.io[addr] = value;
            if (!nes->apu.status.pulse1) nes->apu.pulse1.length_counter_load = 0;
            if (!nes->apu.status.pulse2) nes->apu.pulse2.length_counter_load = 0;
            if (!nes->apu.status.triangle) nes->apu.triangle.length_counter_load = 0;
            if (!nes->apu.status.noise) nes->apu.noise.length_counter_load = 0;
            break;
        case 0x16: /* strobe */
            break;
        case 0x17:
            nes->apu.io[addr] = value;
            break;
    }
}

static inline uint8_t NES_ppu_register_read(struct NES_Core* nes, uint8_t addr) {
    uint8_t data;

    switch (addr & 0x7) {
        case 0x2:
            data = nes->ppu.status;
            nes->ppu._status.vblank = 0;
            break;
        case 0x4:
            data = nes->ppu.oam[nes->ppu.oam_addr];
            break;
        case 0x7:
            data = NES_ppu_read(nes, nes->ppu.addr);
            nes->ppu.addr = (nes->ppu.addr + nes->ppu.addr_inc_value) & 0x3FFF;
            break;
        /* write only regs return current latched value. */
        default:
            data = nes->ppu.latch;
            break;
    }
    
    return data;
}

static inline void NES_ppu_register_write(struct NES_Core* nes, uint8_t addr, uint8_t value) {
    switch (addr & 0x07) {
        case 0x0:
            nes->ppu.ctrl = value;
            nes->ppu.addr_inc_value = nes->ppu._ctrl.vram_addr ? 32 : 1;
            break;
        case 0x1:
            nes->ppu.mask = value;
            break;
        case 0x3:
            nes->ppu.oam_addr = value;
            break;
        case 0x4:
            nes->ppu.oam[nes->ppu.oam_addr++] = value;
            break;
        case 0x5:
        case 0x6:
        case 0x7:
            NES_ppu_write(nes, nes->ppu.addr, value);
            nes->ppu.addr = (nes->ppu.addr + nes->ppu.addr_inc_value) & 0x3FFF;
            break;
    }
}

uint8_t NES_cpu_read(struct NES_Core* nes, uint16_t addr) {
    if (addr <= 0x1FFF) { // ram
        return nes->ram[addr & 0x7FF];
    } else if (addr >= 0x2000 && addr <= 0x3FFF) { // ppu reg
        return NES_ppu_register_read(nes, addr & 0x7);
    } else if (addr >= 0x4000 && addr <= 0x4017) { // io
        return NES_cpu_io_read(nes, addr & 0x1F);
    } else if (addr >= 0x4020) { // cart
        return nes->cart.mapper_read(nes, addr);
    } else {
        NES_log_err("UNK MEM READ: 0x%04X\n", addr);
        assert(0);
    }

    return 0xFF;
}

void NES_cpu_write(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    if (addr <= 0x1FFF) { // ram
        nes->ram[addr & 0x7FF] = value;
    } else if (addr >= 0x2000 && addr <= 0x3FFF) { // ppu reg
        NES_ppu_register_write(nes, addr & 0x7, value);
    } else if (addr >= 0x4000 && addr <= 0x4017) { // io
        NES_cpu_io_write(nes, addr & 0x1F, value);
    }  else if (addr >= 0x4020) { // cart
        nes->cart.mapper_write(nes, addr, value);
    } else {
        NES_log_err("UNK MEM WRITE: 0x%04X\n", addr);
        assert(0);
    }
}

uint16_t NES_cpu_read16(struct NES_Core* nes, uint16_t addr) {
    return (NES_cpu_read(nes, addr)) | (NES_cpu_read(nes, addr + 1) << 8); 
}

void NES_cpu_write16(struct NES_Core* nes, uint16_t addr, uint16_t value) {
    NES_cpu_write(nes, addr, value & 0xFF);
    NES_cpu_write(nes, addr + 1, (value >> 8) & 0xFF);
}

uint8_t NES_ppu_read(struct NES_Core* nes, uint16_t addr) {
    if (addr <= 0x0FFF) {
        return 0xFF;
    } else if (addr >= 0x1000 && addr <= 0x1FFF) { /* pattern table 0 */

    } else if (addr >= 0x2000 && addr <= 0x23FF) { /* pattern table 1 */

    } else if (addr >= 0x24FF && addr <= 0x27FF) { /* nametable 0 */
    
    } else if (addr >= 0x2800 && addr <= 0x2BFF) { /* nametable 1 */

    } else if (addr >= 0x2C00 && addr <= 0x2FFF) { /* nametable 2 */

    } else if (addr >= 0x3000 && addr <= 0x3EFF) { /* mirrors of 0x2000-0x2EFF */

    } else if (addr >= 0x3F20 && addr <= 0x3FFF) { /* palette ram + mirrors. */
        return nes->ppu.pram[addr & 0x1F];
    }

    NES_UNREACHABLE(0xFF);
}

void NES_ppu_write(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    if (addr <= 0x0FFF) {

    } else if (addr >= 0x1000 && addr <= 0x1FFF) { /* pattern table 0 */

    } else if (addr >= 0x2000 && addr <= 0x23FF) { /* pattern table 1 */

    } else if (addr >= 0x24FF && addr <= 0x27FF) { /* nametable 0 */
    
    } else if (addr >= 0x2800 && addr <= 0x2BFF) { /* nametable 1 */

    } else if (addr >= 0x2C00 && addr <= 0x2FFF) { /* nametable 2 */

    } else if (addr >= 0x3000 && addr <= 0x3EFF) { /* mirrors of 0x2000-0x2EFF */

    } else if (addr >= 0x3F20 && addr <= 0x3FFF) { /* palette ram + mirrors. */
        nes->ppu.pram[addr & 0x1F]= value;
    }
}

void NES_dma(struct NES_Core* nes) {
    const uint16_t addr = nes->ppu.oam_addr << 8;
    uint8_t* oam = nes->ppu.oam;

    for (uint16_t i = 0; i < 0x100; i++) {
        oam[i] = NES_cpu_read(nes, addr | i);
    }

    /* 513-514 cycles depending if it ends on odd cycle. */
    /* i don't understand the odd cycle so will just do 514 */
    nes->cpu.cycles += 514;
}
