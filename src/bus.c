#include "nes.h"
#include "internal.h"

#include <stdio.h>
#include <assert.h>


// SOURCE: https://problemkaputt.de/everynes.htm#iomap

static inline uint8_t nes_cpu_io_read(struct NES_Core* nes, uint16_t addr)
{
    uint8_t data;

    switch (addr & 0x1F)
    {
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
            NES_log_fatal("invalid IO read\n");
            UNREACHABLE(0xFF);
    }

    return data;
}

static inline void nes_cpu_io_write(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
    switch (addr & 0x1F)
    {
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
            nes_joypad_write(nes, value);
            break;
    }
}

static inline uint8_t nes_ppu_register_read(struct NES_Core* nes, uint16_t addr)
{
    uint8_t data;

    switch (addr & 0x7)
    {
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
            if (!status_get_vblank(nes))
            {
                NES_log("WARN: reading from oam outside vblank!\n");
            }

            data = nes->ppu.oam[nes->ppu.oam_addr];
            break;

        case 0x7:
            // this returns the previous value as reads are delayed!
            data = nes->ppu.vram_latched_read;
            // save the new value
            nes->ppu.vram_latched_read = nes_ppu_read(nes, nes->ppu.vram_addr);
            // palettes aren't delayed
            if (nes->ppu.vram_addr > 0x3F00)
            {
                data = nes->ppu.vram_latched_read;
            }
            nes->ppu.vram_addr += nes->ppu.vram_addr_increment;
            nes->ppu.vram_addr &= 0x3FFF;
            break;

            /* write only regs return current latched value. */
        default:
            data = nes->ppu.vram_latched_read;
            break;
    }

    return data;
}

static inline void nes_ppu_register_write(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
    switch (addr & 0x07)
    {
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
            if (!status_get_vblank(nes))
            {
                NES_log("WARN: writing to oam outside vblank!\n");
            }

            // the addr is incremented after each write
            nes->ppu.oam[nes->ppu.oam_addr] = value;
            nes->ppu.oam_addr = (nes->ppu.oam_addr + 1) & 0xFF;
            break;

        // scroll stuff
        case 0x5:
            if (nes->ppu.has_first_8bit)
            {
                nes->ppu.vertical_scroll_origin = value;
                nes->ppu.has_first_8bit = false;
            }
            else
            {
                nes->ppu.horizontal_scroll_origin = value;
                nes->ppu.has_first_8bit = true;
            }
            break;

        // vram addr stuff
        case 0x6:
            if (nes->ppu.has_first_8bit)
            {
                // set the new vram addr
                nes->ppu.vram_addr = (nes->ppu.write_flipflop << 8) | value;
                nes->ppu.vram_addr &= 0x3FFF;
                // reset the write_flipflop
                nes->ppu.write_flipflop = 0;
                nes->ppu.has_first_8bit = false;
            }
            else
            {
                // store the MSB to the flipflop
                nes->ppu.write_flipflop = value;
                // also (or only?) seems to write to $2005 MSB
                nes->ppu.horizontal_scroll_origin = value;
                nes->ppu.has_first_8bit = true;
            }
            break;

        case 0x7:
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

uint8_t nes_cpu_read(struct NES_Core* nes, uint16_t addr)
{
    switch ((addr >> 13) & 0x7)
    {
        case 0x0:
            return nes->wram[addr & 0x7FF];

        case 0x1:
            return nes_ppu_register_read(nes, addr);

        case 0x2:
            if (addr <= 0x4017)
            {
                return nes_cpu_io_read(nes, addr);
            }
            else
            {
                return 0xFF; // cart expansion
            }

            case 0x3:
            case 0x4:
            case 0x5:
            case 0x6:
            case 0x7:
                return nes_cart_read(nes, addr);
    }

    UNREACHABLE(0xFF);
}

void nes_cpu_write(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
    switch ((addr >> 13) & 0x7)
    {
        case 0x0:
            nes->wram[addr & 0x7FF] = value;
            break;

        case 0x1:
            nes_ppu_register_write(nes, addr & 0x7, value);
            break;

        case 0x2:
            if (addr <= 0x4017)
            {
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

uint16_t nes_cpu_read16(struct NES_Core* nes, uint16_t addr)
{
    const uint16_t lo = nes_cpu_read(nes, addr + 0);
    const uint16_t hi = nes_cpu_read(nes, addr + 1);

    return lo | (hi << 8);
}

void nes_cpu_write16(struct NES_Core* nes, uint16_t addr, uint16_t value)
{
    nes_cpu_write(nes, addr, value & 0xFF);
    nes_cpu_write(nes, addr + 1, (value >> 8) & 0xFF);
}
