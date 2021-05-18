#include "nes.h"
#include "internal.h"

#include <stdio.h>
#include <assert.h>


// SOURCE: https://problemkaputt.de/everynes.htm#iomap
/*
I/O Map

2000h - PPU Control Register 1 (W)
2001h - PPU Control Register 2 (W)
2002h - PPU Status Register (R)
2003h - SPR-RAM Address Register (W)
2004h - SPR-RAM Data Register (RW)
2005h - PPU Background Scrolling Offset (W2)
2006h - VRAM Address Register (W2)
2007h - VRAM Read/Write Data Register (RW)
4000h - APU Channel 1 (Rectangle) Volume/Decay (W)
4001h - APU Channel 1 (Rectangle) Sweep (W)
4002h - APU Channel 1 (Rectangle) Frequency (W)
4003h - APU Channel 1 (Rectangle) Length (W)
4004h - APU Channel 2 (Rectangle) Volume/Decay (W)
4005h - APU Channel 2 (Rectangle) Sweep (W)
4006h - APU Channel 2 (Rectangle) Frequency (W)
4007h - APU Channel 2 (Rectangle) Length (W)
4008h - APU Channel 3 (Triangle) Linear Counter (W)
4009h - APU Channel 3 (Triangle) N/A (-)
400Ah - APU Channel 3 (Triangle) Frequency (W)
400Bh - APU Channel 3 (Triangle) Length (W)
400Ch - APU Channel 4 (Noise) Volume/Decay (W)
400Dh - APU Channel 4 (Noise) N/A (-)
400Eh - APU Channel 4 (Noise) Frequency (W)
400Fh - APU Channel 4 (Noise) Length (W)
4010h - APU Channel 5 (DMC) Play mode and DMA frequency (W)
4011h - APU Channel 5 (DMC) Delta counter load register (W)
4012h - APU Channel 5 (DMC) Address load register (W)
4013h - APU Channel 5 (DMC) Length register (W)
4014h - SPR-RAM DMA Register (W)
4015h - DMC/IRQ/length counter status/channel enable register (RW)
4016h - Joypad #1 (RW)
4017h - Joypad #2/APU SOFTCLK (RW)
*/

static inline uint8_t nes_cpu_io_read(struct NES_Core* nes, uint16_t addr) {
  uint8_t data;

  switch (addr & 0x1F) {
    case 0x00: case 0x01: case 0x02: case 0x03:
    case 0x04: case 0x05: case 0x06: case 0x07:
    case 0x08: case 0x09: case 0x0A: case 0x0B:
    case 0x0C: case 0x0D: case 0x0E: case 0x0F:
    case 0x10: case 0x11: case 0x12: case 0x13:
    case 0x15:
      return nes_apu_io_read(nes, addr);

    case 0x14:
      data = nes->ppu.oam_addr;
      break;

    case 0x16: /* controller 1 */
      data = nes_joypad_read_port_0(nes);
      break;

    case 0x17: /* controller 2 */
      data = 0x00;
      break;

    default:
      assert(0 && "invalid IO read");
      NES_UNREACHABLE(0xFF);
  }

  return data;
}

static inline void nes_cpu_io_write(struct NES_Core* nes, uint16_t addr, uint8_t value) {
  switch (addr & 0x1F) {
    case 0x00: case 0x01: case 0x02: case 0x03:
    case 0x04: case 0x05: case 0x06: case 0x07:
    case 0x08: case 0x09: case 0x0A: case 0x0B:
    case 0x0C: case 0x0D: case 0x0E: case 0x0F:
    case 0x10: case 0x11: case 0x12: case 0x13:
    case 0x15: case 0x17:
      nes_apu_io_write(nes, addr, value);
      break;

    case 0x14:
      nes->ppu.oam_addr = value;
      nes_dma(nes);
      break;

    case 0x16: /* strobe */
      nes->jp.strobe = value & 0x1;

      if (nes->jp.strobe) {
        nes->jp.latch_a = nes->jp.buttons_a;
        nes->jp.latch_b = nes->jp.buttons_b;
      }

      break;
  }
}

static inline uint8_t nes_ppu_register_read(struct NES_Core* nes, uint16_t addr) {
  uint8_t data;

  switch (addr & 0x7) {
    case 0x2:
      data = nes->ppu.status;
      // reading from this register also resets the flipflop
      // does this reset $2005 and $2006 as well?
      nes->ppu.write_flipflop = 0;
      nes->ppu.has_first_8bit = false;
      // reading resets the 7-bit, which is the vblank flag
      status_set_vblank(nes, false);
      break;

    case 0x4:
      if (!status_get_vblank(nes)) {
        nes_log("WARN: reading from oam outside vblank!\n");
      }

      data = nes->ppu.oam[nes->ppu.oam_addr];
      break;

    case 0x7:
      if (!status_get_vblank(nes)) {
        nes_log("WARN: reading from vram outside vblank!\n");
        return 0xFF; // maybe it returns latch?
      }

      // this returns the previous value as reads are delayed!
      data = nes->ppu.vram_latched_read;
      // save the new value
      nes->ppu.vram_latched_read = nes_ppu_read(nes, nes->ppu.vram_addr);
      // palettes aren't delayed
      if (nes->ppu.vram_addr > 0x3F00) {
          data = nes->ppu.vram_latched_read;
      }
      break;

    /* write only regs return current latched value. */
    default:
      data = nes->ppu.vram_latched_read;
      break;
  }

  return data;
}

static inline void nes_ppu_register_write(struct NES_Core* nes, uint16_t addr, uint8_t value) {
  switch (addr & 0x07) {
    case 0x0:
      nes->ppu.ctrl = value;
      // this inc is used for $2007 when writing, the addr is incremented
      nes->ppu.vram_addr_increment = ctrl_get_vram_addr(nes) ? 32 : 1;
      break;

    case 0x1:
      nes->ppu.mask = value;
      break;

    case 0x3:
      // this addr is used for indexing the oam
      nes->ppu.oam_addr = value;
      break;

    case 0x4:
      if (!status_get_vblank(nes)) {
        nes_log("WARN: writing to oam outside vblank!\n");
      }

      // the addr is incremented after each write
      nes->ppu.oam[nes->ppu.oam_addr] = value;
      nes->ppu.oam_addr = (nes->ppu.oam_addr + 1) & 0xFF;
      break;

    // scroll stuff
    case 0x5:
      if (nes->ppu.has_first_8bit) {
        nes->ppu.vertical_scroll_origin = value;
        nes->ppu.has_first_8bit = false;
      }
      else {
        nes->ppu.horizontal_scroll_origin = value;
        nes->ppu.has_first_8bit = true;
      }
      break;

    // vram addr stuff
    case 0x6:
      /* TODO:
        VRAM-Pointer            Scroll-Reload
          A8  2006h/1st-Bit0 <--> Y*64  2005h/2nd-Bit6
          A9  2006h/1st-Bit1 <--> Y*128 2005h/2nd-Bit7
          A10 2006h/1st-Bit2 <--> X*256 2000h-Bit0
          A11 2006h/1st-Bit3 <--> Y*240 2000h-Bit1
          A12 2006h/1st-Bit4 <--> Y*1   2005h/2nd-Bit0
          A13 2006h/1st-Bit5 <--> Y*2   2005h/2nd-Bit1
          -   2006h/1st-Bit6 <--> Y*4   2005h/2nd-Bit2
          -   2006h/1st-Bit7 <--> -     -
          A0  2006h/2nd-Bit0 <--> X*8   2005h/1st-Bit3
          A1  2006h/2nd-Bit1 <--> X*16  2005h/1st-Bit4
          A2  2006h/2nd-Bit2 <--> X*32  2005h/1st-Bit5
          A3  2006h/2nd-Bit3 <--> X*64  2005h/1st-Bit6
          A4  2006h/2nd-Bit4 <--> X*128 2005h/1st-Bit7
          A5  2006h/2nd-Bit5 <--> Y*8   2005h/2nd-Bit3
          A6  2006h/2nd-Bit6 <--> Y*16  2005h/2nd-Bit4
          A7  2006h/2nd-Bit7 <--> Y*32  2005h/2nd-Bit5
          -   -              <--> X*1   2005h/1st-Bit0
          -   -              <--> X*2   2005h/1st-Bit1
          -   -              <--> X*4   2005h/1st-Bit2
      */
      if (nes->ppu.has_first_8bit) {
        // set the new vram addr
        nes->ppu.vram_addr = (nes->ppu.write_flipflop << 8) | value;
        assert(nes->ppu.vram_addr <= 0x3FFF);
        // reset the write_flipflop
        nes->ppu.write_flipflop = 0;
        nes->ppu.has_first_8bit = false;
      }
      else {
        // store the MSB to the flipflop
        nes->ppu.write_flipflop = value;
        // also (or only?) seems to write to $2005 MSB
        nes->ppu.horizontal_scroll_origin = value;
        nes->ppu.has_first_8bit = true;
      }
      break;

    case 0x7:
      if (!status_get_vblank(nes)) {
        nes_log("WARN: writing to vram outside vblank!\n");
        return;
      }

      nes_ppu_write(nes, nes->ppu.vram_addr, value);
      nes->ppu.vram_addr += nes->ppu.vram_addr_increment;
      nes->ppu.vram_addr &= 0x3FFF;
      break;
  }
}


// SOURCE: https://problemkaputt.de/everynes.htm#memorymaps
/*
CPU Memory Map (16bit buswidth, 0-FFFFh)

0000h-07FFh   Internal 2K Work RAM (mirrored to 800h-1FFFh)
2000h-2007h   Internal PPU Registers (mirrored to 2008h-3FFFh)
4000h-4017h   Internal APU Registers
4018h-5FFFh   Cartridge Expansion Area almost 8K
6000h-7FFFh   Cartridge SRAM Area 8K
8000h-FFFFh   Cartridge PRG-ROM Area 32K
*/

uint8_t nes_cpu_read(struct NES_Core* nes, uint16_t addr) {
  switch ((addr >> 13) & 0x7) {
    case 0x0:
      return nes->wram[addr & 0x7FF];

    case 0x1:
      return nes_ppu_register_read(nes, addr);
    
    case 0x2:
      if (addr <= 0x4017) {
        return nes_cpu_io_read(nes, addr);
      }
      return 0xFF; // cart expansion
    
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
      return nes_cart_read(nes, addr);
  }

  NES_UNREACHABLE(0xFF);
}

void nes_cpu_write(struct NES_Core* nes, uint16_t addr, uint8_t value) {
  switch ((addr >> 13) & 0x7) {
    case 0x0:
      nes->wram[addr & 0x7FF] = value;
      break;

    case 0x1:
      nes_ppu_register_write(nes, addr & 0x7, value);
      break;
    
    case 0x2:
      if (addr <= 0x4017) {
        nes_cpu_io_write(nes, addr, value);
      }
      break;
    
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
      nes_cart_write(nes, addr, value);
      break;
  }
}

uint16_t nes_cpu_read16(struct NES_Core* nes, uint16_t addr) {
  const uint16_t lo = nes_cpu_read(nes, addr + 0);
  const uint16_t hi = nes_cpu_read(nes, addr + 1);

  return lo | (hi << 8);
}

void nes_cpu_write16(struct NES_Core* nes, uint16_t addr, uint16_t value) {
  nes_cpu_write(nes, addr, value & 0xFF);
  nes_cpu_write(nes, addr + 1, (value >> 8) & 0xFF);
}
