#include "core/nes.h"
#include "core/internal.h"


#include <stdio.h>
#include <assert.h>


struct RgbTriple {
    uint8_t r, g, b;
};

// todo: generate this table again but use BGR555 instead
static struct RgbTriple NES_RGB888_PALETTE[0x40] = {
    [0x00] = { .r = 0x54, .g = 0x54, .b = 0x54 },
    [0x01] = { .r = 0x00, .g = 0x1E, .b = 0x74 },
    [0x02] = { .r = 0x08, .g = 0x10, .b = 0x90 },
    [0x03] = { .r = 0x30, .g = 0x00, .b = 0x88 },
    [0x04] = { .r = 0x44, .g = 0x00, .b = 0x64 },
    [0x05] = { .r = 0x5C, .g = 0x00, .b = 0x48 },
    [0x06] = { .r = 0x54, .g = 0x04, .b = 0x00 },
    [0x07] = { .r = 0x3C, .g = 0x18, .b = 0x00 },
    [0x08] = { .r = 0x20, .g = 0x2A, .b = 0x00 },
    [0x09] = { .r = 0x08, .g = 0x3A, .b = 0x00 },
    [0x0A] = { .r = 0x00, .g = 0x40, .b = 0x00 },
    [0x0B] = { .r = 0x00, .g = 0x3C, .b = 0x00 },
    [0x0C] = { .r = 0x00, .g = 0x32, .b = 0x3C },
    [0x0D] = { .r = 0x00, .g = 0x00, .b = 0x00 },
    [0x0E] = { .r = 0x00, .g = 0x00, .b = 0x00 },
    [0x0F] = { .r = 0x00, .g = 0x00, .b = 0x00 },

    [0x10] = { .r = 152, .g = 150, .b = 152 },
    [0x11] = { .r = 8, .g = 76, .b = 196 },
    [0x12] = { .r = 48, .g = 50, .b = 236 },
    [0x13] = { .r = 92, .g = 30, .b = 228 },
    [0x14] = { .r = 136, .g = 20, .b = 176 },
    [0x15] = { .r = 160, .g = 20, .b = 100 },
    [0x16] = { .r = 152, .g = 34, .b = 32 },
    [0x17] = { .r = 120, .g = 60, .b = 0 },
    [0x18] = { .r = 84, .g = 90, .b = 0 },
    [0x19] = { .r = 40, .g = 114, .b = 0 },
    [0x1A] = { .r = 8, .g = 124, .b = 0 },
    [0x1B] = { .r = 0, .g = 118, .b = 40 },
    [0x1C] = { .r = 0, .g = 102, .b = 120 },
    [0x1D] = { .r = 0x00, .g = 0x00, .b = 0x00 },
    [0x1E] = { .r = 0x00, .g = 0x00, .b = 0x00 },
    [0x1F] = { .r = 0x00, .g = 0x00, .b = 0x00 },

    [0x20] = { .r = 236, .g = 238, .b = 236 },
    [0x21] = { .r = 76, .g = 154, .b = 236 },
    [0x22] = { .r = 120, .g = 124, .b = 236 },
    [0x23] = { .r = 176, .g = 98, .b = 236 },
    [0x24] = { .r = 228, .g = 84, .b = 236 },
    [0x25] = { .r = 236, .g = 88, .b = 180 },
    [0x26] = { .r = 236, .g = 106, .b = 100 },
    [0x27] = { .r = 212, .g = 136, .b = 32 },
    [0x28] = { .r = 160, .g = 170, .b = 0x00 },
    [0x29] = { .r = 116, .g = 196, .b = 0x00 },
    [0x2A] = { .r = 76, .g = 208, .b = 32 },
    [0x2B] = { .r = 56, .g = 204, .b = 108 },
    [0x2C] = { .r = 56, .g = 180, .b = 204 },
    [0x2D] = { .r = 60, .g = 60, .b = 60 },
    [0x2E] = { .r = 0x00, .g = 0x00, .b = 0x00 },
    [0x2F] = { .r = 0x00, .g = 0x00, .b = 0x00 },

    [0x30] = { .r = 236, .g = 238, .b = 236 },
    [0x31] = { .r = 168, .g = 204, .b = 236 },
    [0x32] = { .r = 188, .g = 188, .b = 236 },
    [0x33] = { .r = 212, .g = 178, .b = 236 },
    [0x34] = { .r = 236, .g = 174, .b = 236 },
    [0x35] = { .r = 236, .g = 174, .b = 212 },
    [0x36] = { .r = 236, .g = 180, .b = 176 },
    [0x37] = { .r = 228, .g = 196, .b = 144 },
    [0x38] = { .r = 204, .g = 210, .b = 120 },
    [0x39] = { .r = 180, .g = 222, .b = 120 },
    [0x3A] = { .r = 168, .g = 226, .b = 144 },
    [0x3B] = { .r = 152, .g = 226, .b = 180 },
    [0x3C] = { .r = 160, .g = 214, .b = 228 },
    [0x3D] = { .r = 160, .g = 162, .b = 160 },
    [0x3E] = { .r = 0x00, .g = 0x00, .b = 0x00 },
    [0x3F] = { .r = 0x00, .g = 0x00, .b = 0x00 },
};


// SOURCE: https://problemkaputt.de/everynes.htm#memorymaps
/*
PPU Memory Map (14bit buswidth, 0-3FFFh)

  0000h-0FFFh   Pattern Table 0 (4K) (256 Tiles)
  1000h-1FFFh   Pattern Table 1 (4K) (256 Tiles)
  2000h-23FFh   Name Table 0 and Attribute Table 0 (1K) (32x30 BG Map)
  2400h-27FFh   Name Table 1 and Attribute Table 1 (1K) (32x30 BG Map)
  2800h-2BFFh   Name Table 2 and Attribute Table 2 (1K) (32x30 BG Map)
  2C00h-2FFFh   Name Table 3 and Attribute Table 3 (1K) (32x30 BG Map)
  3000h-3EFFh   Mirror of 2000h-2EFFh
  3F00h-3F1Fh   Background and Sprite Palettes (25 entries used)
  3F20h-3FFFh   Mirrors of 3F00h-3F1Fh
*/
uint8_t NES_ppu_read(struct NES_Core* nes, uint16_t addr) {
    if (addr <= 0x0FFF) { /* pattern table 0 */
        return NES_cart_read(nes, addr);
    }

    else if (addr >= 0x1000 && addr <= 0x1FFF) { /* pattern table 1 */
        return NES_cart_read(nes, addr);
    }
    
    else if (addr >= 0x2000 && addr <= 0x23FF) { /* nametable 0 */
        return 0xFF;
    }
    
    else if (addr >= 0x2400 && addr <= 0x27FF) { /* nametable 1 */
        return 0xFF;
    }
    
    else if (addr >= 0x2800 && addr <= 0x2BFF) { /* nametable 2 */
        return 0xFF;
    }
    
    else if (addr >= 0x2C00 && addr <= 0x2FFF) { /* nametable 3 */
        return 0xFF;
    }
    
    else if (addr >= 0x3000 && addr <= 0x3EFF) { /* mirrors of 0x2000-0x2EFF */
        return NES_ppu_read(nes, addr - 0x1000);
    }
    
    else if (addr >= 0x3F00 && addr <= 0x3FFF) { /* palette ram + mirrors. */
        return nes->ppu.pram[addr & 0x1F];
    }

    assert(0);
    NES_UNREACHABLE(0xFF);
}

void NES_ppu_write(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    if (addr <= 0x0FFF) { /* pattern table 0 */
        NES_cart_write(nes, addr, value);
    }
    
    else if (addr >= 0x1000 && addr <= 0x1FFF) { /* pattern table 1 */
        NES_cart_write(nes, addr, value);
    }
    
    else if (addr >= 0x2000 && addr <= 0x23FF) { /* nametable 0 */

    }
    
    else if (addr >= 0x2400 && addr <= 0x27FF) { /* nametable 1 */
    
    }
    
    else if (addr >= 0x2800 && addr <= 0x2BFF) { /* nametable 2 */

    }
    
    else if (addr >= 0x2C00 && addr <= 0x2FFF) { /* nametable 3 */

    }
    
    else if (addr >= 0x3000 && addr <= 0x3EFF) { /* mirrors of 0x2000-0x2EFF */
        NES_ppu_write(nes, addr - 0x1000, value);
    }
    
    else if (addr >= 0x3F00 && addr <= 0x3FFF) { /* palette ram + mirrors. */
        nes->ppu.pram[addr & 0x1F] = value;
    }
}

// this is called by the cpu when writing to $4014 register
void NES_dma(struct NES_Core* nes) {
    const uint16_t addr = nes->ppu.oam_addr << 8;
    uint8_t* oam = nes->ppu.oam;

    // fills the entire oam!
    for (uint16_t i = 0; i < 0x100; i++) {
        oam[i] = NES_cpu_read(nes, addr | i);
    }

    /* everynes says this takes 512 clock cycles. */
    // don't clock these cycles for now as i think it'll
    // cause problems with the ppu timing.
    // nes->cpu.cycles += 512;
}

static struct RgbTriple get_colour_from_palette_ram(struct NES_Core* nes, uint8_t palette, uint8_t colour_id) {
    uint8_t offset = (palette * 4) + colour_id;
    uint8_t index = nes->ppu.pram[offset & 0x1F];
    assert(index <= 0x3F);
    return NES_RGB888_PALETTE[index & 0x3F];
}

// thank you OLC!
// SOURCE: https://www.youtube.com/watch?v=-THeUXqR3zY&t=1759s
struct NES_PatternTableGfx NES_ppu_get_pattern_table(struct NES_Core* nes, uint8_t table_index, uint8_t palette) {
    struct NES_PatternTableGfx gfx_output = {0};

    // make sure we don't overflow
    palette &= 0x07;
    const uint16_t pattern_table_index = (table_index & 1) * 0x1000;

    // 16x16
    for (uint16_t y = 0; y < 16; ++y) {

        for (uint16_t x = 0; x < 16; ++x) {

            const uint16_t offset = (y * 256) + (x * 16);

            for (uint16_t row = 0; row < 8; ++row) {

                uint16_t lsb = NES_ppu_read(nes,
                    pattern_table_index + offset + row + 0
                );
                uint16_t msb = NES_ppu_read(nes,
                    pattern_table_index + offset + row + 8
                );

                for (uint16_t coloumn = 0; coloumn < 8; ++coloumn) {
                    const uint8_t colour_id = (lsb & 1) + (msb & 1);

                    const struct RgbTriple rgb_triple = get_colour_from_palette_ram(nes, palette, colour_id);
                    
                    const uint32_t colour = (rgb_triple.r << 16) | (rgb_triple.g << 8) | (rgb_triple.b);
                    
                    gfx_output.pixels[y * 8 + row][x * 8 + (7 - coloumn)] = colour;

                    // shift down
                    lsb >>= 1;
                    msb >>= 1;
                }
            }
        }
    }

    return gfx_output;
}

// there are 262 scanlines total
// each scanline takes 341 ppu clock, so ~113 cpu clocks
// a pixel is created every clock cycle (ppu cycle?)
void NES_ppu_run(struct NES_Core* nes, const uint16_t cycles_elapsed) {
    nes->ppu.next_cycles -= cycles_elapsed;

    ++nes->ppu.cycles;

    if (nes->ppu.cycles >= 341) {
        nes->ppu.cycles = 0;
        ++nes->ppu.scaline;

        // vblank
        if (nes->ppu.scaline == 240) {
            // set the status to vblank
            nes->ppu._status.vblank = 1;

            // check if the nmi bit is set, if so then fire an nmi.
            if (nes->ppu._ctrl.nmi) {
                NES_cpu_nmi(nes);
            }
        }

        // reset
        else if (nes->ppu.scaline == 261) {
            // we are no longer in vblank, always clear the flag
            nes->ppu._status.vblank = 0;
            nes->ppu.scaline = -1;
        }
    }
}
