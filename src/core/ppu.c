#include "nes.h"
#include "internal.h"


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

void ctrl_set_nametable(struct NES_Core* nes, uint8_t v) {
  nes->ppu.ctrl = (nes->ppu.ctrl & ~(0x03 << 0)) | ((v & 0x03) << 0);
}

uint8_t ctrl_get_nametable(const struct NES_Core* nes) {
  return (nes->ppu.ctrl >> 0) & 0x03;
}

void ctrl_set_vram_addr(struct NES_Core* nes, uint8_t v) {
  nes->ppu.ctrl = (nes->ppu.ctrl & ~(0x01 << 2)) | ((v & 0x01) << 2);
}

uint8_t ctrl_get_vram_addr(const struct NES_Core* nes) {
  return (nes->ppu.ctrl >> 2) & 0x01;
}

void ctrl_set_obj_8x8_addr(struct NES_Core* nes, uint8_t v) {
  nes->ppu.ctrl = (nes->ppu.ctrl & ~(0x01 << 3)) | ((v & 0x01) << 3);
}

uint8_t ctrl_get_obj_8x8_addr(const struct NES_Core* nes) {
  return (nes->ppu.ctrl >> 3) & 0x01;
}

void ctrl_set_bg_addr(struct NES_Core* nes, uint8_t v) {
  nes->ppu.ctrl = (nes->ppu.ctrl & ~(0x01 << 4)) | ((v & 0x01) << 4);
}

uint8_t ctrl_get_bg_addr(const struct NES_Core* nes) {
  return (nes->ppu.ctrl >> 4) & 0x01;
}

void ctrl_set_obj_size(struct NES_Core* nes, uint8_t v) {
  nes->ppu.ctrl = (nes->ppu.ctrl & ~(0x01 << 5)) | ((v & 0x01) << 5);
}

uint8_t ctrl_get_obj_size(const struct NES_Core* nes) {
  return (nes->ppu.ctrl >> 5) & 0x01;
}

void ctrl_set_master(struct NES_Core* nes, uint8_t v) {
  nes->ppu.ctrl = (nes->ppu.ctrl & ~(0x01 << 6)) | ((v & 0x01) << 6);
}

uint8_t ctrl_get_master(const struct NES_Core* nes) {
  return (nes->ppu.ctrl >> 6) & 0x01;
}

void ctrl_set_nmi(struct NES_Core* nes, uint8_t v) {
  nes->ppu.ctrl = (nes->ppu.ctrl & ~(0x01 << 7)) | ((v & 0x01) << 7);
}

uint8_t ctrl_get_nmi(const struct NES_Core* nes) {
  return (nes->ppu.ctrl >> 7) & 0x01;
}

void mask_set_greyscale(struct NES_Core* nes, uint8_t v) {
  nes->ppu.mask = (nes->ppu.mask & ~(0x01 << 0)) | ((v & 0x01) << 0);
}

uint8_t mask_get_greyscale(const struct NES_Core* nes) {
  return (nes->ppu.mask >> 0) & 0x01;
}

void mask_set_bg_leftmost(struct NES_Core* nes, uint8_t v) {
  nes->ppu.mask = (nes->ppu.mask & ~(0x01 << 1)) | ((v & 0x01) << 1);
}

uint8_t mask_get_bg_leftmost(const struct NES_Core* nes) {
  return (nes->ppu.mask >> 1) & 0x01;
}

void mask_set_obj_leftmost(struct NES_Core* nes, uint8_t v) {
  nes->ppu.mask = (nes->ppu.mask & ~(0x01 << 2)) | ((v & 0x01) << 2);
}

uint8_t mask_get_obj_leftmost(const struct NES_Core* nes) {
  return (nes->ppu.mask >> 2) & 0x01;
}

void mask_set_bg_on(struct NES_Core* nes, uint8_t v) {
  nes->ppu.mask = (nes->ppu.mask & ~(0x01 << 3)) | ((v & 0x01) << 3);
}

uint8_t mask_get_bg_on(const struct NES_Core* nes) {
  return (nes->ppu.mask >> 3) & 0x01;
}

void mask_set_obj_on(struct NES_Core* nes, uint8_t v) {
  nes->ppu.mask = (nes->ppu.mask & ~(0x01 << 4)) | ((v & 0x01) << 4);
}

uint8_t mask_get_obj_on(const struct NES_Core* nes) {
  return (nes->ppu.mask >> 4) & 0x01;
}

void mask_set_bgr(struct NES_Core* nes, uint8_t v) {
  nes->ppu.mask = (nes->ppu.mask & ~(0x07 << 5)) | ((v & 0x07) << 5);
}

uint8_t mask_get_bgr(const struct NES_Core* nes) {
  return (nes->ppu.mask >> 5) & 0x07;
}

void status_set_lsb(struct NES_Core* nes, uint8_t v) {
  nes->ppu.status = (nes->ppu.status & ~(0x1F << 0)) | ((v & 0x1F) << 0);
}

uint8_t status_get_lsb(const struct NES_Core* nes) {
  return (nes->ppu.status >> 0) & 0x1F;
}

void status_set_obj_overflow(struct NES_Core* nes, uint8_t v) {
  nes->ppu.status = (nes->ppu.status & ~(0x01 << 5)) | ((v & 0x01) << 5);
}

uint8_t status_get_obj_overflow(const struct NES_Core* nes) {
  return (nes->ppu.status >> 5) & 0x01;
}

void status_set_obj_hit(struct NES_Core* nes, uint8_t v) {
  nes->ppu.status = (nes->ppu.status & ~(0x01 << 6)) | ((v & 0x01) << 6);
}

uint8_t status_get_obj_hit(const struct NES_Core* nes) {
  return (nes->ppu.status >> 6) & 0x01;
}

void status_set_vblank(struct NES_Core* nes, uint8_t v) {
  nes->ppu.status = (nes->ppu.status & ~(0x01 << 7)) | ((v & 0x01) << 7);
}

uint8_t status_get_vblank(const struct NES_Core* nes) {
  return (nes->ppu.status >> 7) & 0x01;
}

// nametable is 0x3C0 bytes, then 0x40 bytes of attributes
struct BgAttribute {
  uint8_t upper_left  /*: 2*/;
  uint8_t upper_right /*: 2*/;
  uint8_t lower_left  /*: 2*/;
  uint8_t lower_right /*: 2*/;
};

// as its 64 bytes next to each other for attr data,
// could use simd to set all the data into the array
static inline struct BgAttribute gen_bg_attr(const uint8_t v) {
  return (struct BgAttribute){
    .upper_left   = (v >> 0) & 0x3,
    .upper_right  = (v >> 2) & 0x3,
    .lower_left   = (v >> 4) & 0x3,
    .lower_right  = (v >> 6) & 0x3
  };
}

struct ObjAttribute {
  bool vertical_flip;
  bool horizontal_flip;
  bool bg_prio;
  uint8_t palette;
};

struct Obj {
  uint8_t y;
  uint8_t num;
  struct ObjAttribute attr;
  uint8_t x;
};

static inline struct ObjAttribute gen_ob_attr(const uint8_t v) {
  return (struct ObjAttribute){
    .vertical_flip    = (v >> 7) & 0x1,
    .horizontal_flip  = (v >> 6) & 0x1,
    .bg_prio          = (v >> 5) & 0x1,
    .palette          = (v >> 0) & 0x3,
  };
}

void ppu_reset(struct NES_Core* nes) {
  (void)nes;
}


// SOURCE: https://problemkaputt.de/everynes.htm#ppureset
// in order to support the locked registers, i can add a mask
// table for every ppu reg, in which at statup, all writes
// will be masked with 0, so the reg doesn't change.
// then after the first frame, set the masks to 0xFF.


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

// [PPU MEMORY MAP]
// [0x0] = pattern table 0
// [0x1] = pattern table 0
// [0x2] = pattern table 0
// [0x3] = pattern table 0
// [0x4] = pattern table 1
// [0x5] = pattern table 1
// [0x6] = pattern table 1
// [0x7] = pattern table 1
// [0x8] = name table 0
// [0x9] = name table 1
// [0xA] = name table 2
// [0xB] = name table 3
// [0xC] = name table 0
// [0xD] = name table 1
// [0xE] = name table 2
// [0xF] = name table 3

uint8_t nes_ppu_read(struct NES_Core* nes, uint16_t addr) {
  assert(addr <= 0x3FFF);

  // if (addr <= 0x3EFF) {
  //   return nes->ppu.map[addr >> 10][addr & 0x3FF];
  // }
  // else {
  //   return nes->ppu.pram[addr & 0x1F];
  // }

  if (addr <= 0x0FFF) { /* pattern table 0 */
    return nes_cart_read(nes, addr);
  }

  else if (addr >= 0x1000 && addr <= 0x1FFF) { /* pattern table 1 */
    return nes_cart_read(nes, addr);
  }

  else if (addr >= 0x2000 && addr <= 0x23FF) { /* nametable 0 */
    return nes->ppu.vram[addr - 0x2000];
  }

  else if (addr >= 0x2400 && addr <= 0x27FF) { /* nametable 1 */
    return nes->ppu.vram[addr - 0x2000];
  }

  else if (addr >= 0x2800 && addr <= 0x2BFF) { /* nametable 2 */
    return nes->ppu.vram[addr - 0x2800];
  }

  else if (addr >= 0x2C00 && addr <= 0x2FFF) { /* nametable 3 */
    return nes->ppu.vram[addr - 0x2800];
  }

  else if (addr >= 0x3000 && addr <= 0x3EFF) { /* mirrors of 0x2000-0x2EFF */
    return nes_ppu_read(nes, addr - 0x1000);
  }

  else if (addr >= 0x3F00 && addr <= 0x3FFF) { /* palette ram + mirrors. */
    return nes->ppu.pram[addr & 0x1F] | 0xC0;
  }

  NES_UNREACHABLE(0xFF);
}

void nes_ppu_write(struct NES_Core* nes, uint16_t addr, uint8_t value) {
  assert(addr <= 0x3FFF);

  // if (addr <= 0x3EFF) {
  //   nes->ppu.map[addr >> 10][addr & 0x3FF] = value;
  // }
  // else {
  //   nes->ppu.pram[addr & 0x1F] = value;
  // }

  if (addr <= 0x0FFF) { /* pattern table 0 */
    nes_cart_write(nes, addr, value);
  }

  else if (addr >= 0x1000 && addr <= 0x1FFF) { /* pattern table 1 */
    nes_cart_write(nes, addr, value);
  }

  else if (addr >= 0x2000 && addr <= 0x23FF) { /* nametable 0 */
    nes->ppu.vram[addr - 0x2000] = value;
  }

  else if (addr >= 0x2400 && addr <= 0x27FF) { /* nametable 1 */
    nes->ppu.vram[addr - 0x2000] = value;
  }

  else if (addr >= 0x2800 && addr <= 0x2BFF) { /* nametable 2 */
    nes->ppu.vram[addr - 0x2800] = value;
  }

  else if (addr >= 0x2C00 && addr <= 0x2FFF) { /* nametable 3 */
    nes->ppu.vram[addr - 0x2800] = value;
  }

  else if (addr >= 0x3000 && addr <= 0x3EFF) { /* mirrors of 0x2000-0x2EFF */
    nes_ppu_write(nes, addr - 0x1000, value);
  }

  else if (addr >= 0x3F00 && addr <= 0x3FFF) { /* palette ram + mirrors. */
    nes->ppu.pram[addr & 0x1F] = value;
  }
}

// this is called by the cpu when writing to $4014 register
void nes_dma(struct NES_Core* nes) {
  const uint16_t addr = nes->ppu.oam_addr << 8;
  uint8_t* oam = nes->ppu.oam;

  // fills the entire oam!
  for (uint16_t i = 0; i < 0x100; i++) {
    oam[i] = nes_cpu_read(nes, addr | i);
  }

  /* everynes says this takes 512 clock cycles. */
  // ~~don't clock these cycles for now as i think it'll
  // cause problems with the ppu timing.~~
  nes->cpu.cycles += 512;
}

static void render_bg(struct NES_Core* nes, int line) {
  // the pattern table 0/1 is selected by bit-4 in ctrl reg.
  // for now, this impl is slow, until i add array maps!
  const uint16_t pattern_table_addr = ctrl_get_bg_addr(nes) == 0 ? 0 : 0x1000;

  (void)line;
  (void)pattern_table_addr;
}

static void render_obj(struct NES_Core* nes, int line) {
  struct Obj objs[64];

  for (unsigned i = 0; i < 64; ++i) {
    objs[i].y     = nes->ppu.oam[(i * 4) + 0];
    objs[i].num   = nes->ppu.oam[(i * 4) + 1];
    objs[i].attr  = gen_ob_attr(nes->ppu.oam[(i * 4) + 2]);
    objs[i].x     = nes->ppu.oam[(i * 4) + 3];
  }

  (void)line;
  (void)objs;
}

static void render_scanline(struct NES_Core* nes, int line) {
  if (line >= 240 || line < 0) {
    return;
  }

  // need to draw 256 pixels wide!
  render_bg(nes, line);
  render_obj(nes, line);
}

// there are 262 scanlines total
// each scanline takes 341 ppu clock, so ~113 cpu clocks
// a pixel is created every clock cycle (ppu cycle?)
void nes_ppu_run(struct NES_Core* nes, const uint16_t cycles_elapsed) {
  (void)cycles_elapsed;
  (void)NES_RGB888_PALETTE;
  
  ++nes->ppu.cycles;

  if (nes->ppu.cycles >= 341) {
    render_scanline(nes, nes->ppu.scanline);

    nes->ppu.cycles = 0;
    ++nes->ppu.scanline;

    // vblank
    if (nes->ppu.scanline == 240) {
      // set the status to vblank
      status_set_vblank(nes, true);

      // check if the nmi bit is set, if so then fire an nmi.
      if (ctrl_get_nmi(nes)) {
        nes_cpu_nmi(nes);
      }
    }

    // reset
    else if (nes->ppu.scanline == 261) {
      // we are no longer in vblank, always clear the flag
      status_set_vblank(nes, false);
      nes->ppu.scanline = -1;
    }
  }
}
