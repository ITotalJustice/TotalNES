#include "core/nes.h"
#include "core/internal.h"


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
    if (addr <= 0x0FFF) {
        return 0xFF;
    }

    else if (addr >= 0x1000 && addr <= 0x1FFF) { /* pattern table 0 */

    }
    
    else if (addr >= 0x2000 && addr <= 0x23FF) { /* pattern table 1 */

    }
    
    else if (addr >= 0x24FF && addr <= 0x27FF) { /* nametable 0 */
    
    }
    
    else if (addr >= 0x2800 && addr <= 0x2BFF) { /* nametable 1 */

    }
    
    else if (addr >= 0x2C00 && addr <= 0x2FFF) { /* nametable 2 */

    }
    
    else if (addr >= 0x3000 && addr <= 0x3EFF) { /* mirrors of 0x2000-0x2EFF */

    }
    
    else if (addr >= 0x3F20 && addr <= 0x3FFF) { /* palette ram + mirrors. */
        return nes->ppu.pram[addr & 0x1F];
    }

    NES_UNREACHABLE(0xFF);
}

void NES_ppu_write(struct NES_Core* nes, uint16_t addr, uint8_t value) {
    if (addr <= 0x0FFF) {

    }
    
    else if (addr >= 0x1000 && addr <= 0x1FFF) { /* pattern table 0 */

    }
    
    else if (addr >= 0x2000 && addr <= 0x23FF) { /* pattern table 1 */

    }
    
    else if (addr >= 0x24FF && addr <= 0x27FF) { /* nametable 0 */
    
    }
    
    else if (addr >= 0x2800 && addr <= 0x2BFF) { /* nametable 1 */

    }
    
    else if (addr >= 0x2C00 && addr <= 0x2FFF) { /* nametable 2 */

    }
    
    else if (addr >= 0x3000 && addr <= 0x3EFF) { /* mirrors of 0x2000-0x2EFF */

    }
    
    else if (addr >= 0x3F20 && addr <= 0x3FFF) { /* palette ram + mirrors. */
        nes->ppu.pram[addr & 0x1F]= value;
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
    nes->cpu.cycles += 512;
}

void NES_ppu_run(struct NES_Core* nes) {   
}
