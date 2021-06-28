#include "nes.h"
#include "internal.h"

#include <stdio.h>
#include <assert.h>


uint8_t ctrl_get_vram_addr(const struct NES_Core* nes)
{
    return (nes->ppu.ctrl >> 2) & 0x01;
}

void ctrl_set_nmi(struct NES_Core* nes, uint8_t v)
{
    nes->ppu.ctrl = (nes->ppu.ctrl & ~(0x01 << 7)) | ((v & 0x01) << 7);
}

uint8_t ctrl_get_nmi(const struct NES_Core* nes)
{
    return (nes->ppu.ctrl >> 7) & 0x01;
}

uint8_t mask_get_bg_leftmost(const struct NES_Core* nes)
{
    return (nes->ppu.mask >> 1) & 0x01;
}

uint8_t mask_get_obj_leftmost(const struct NES_Core* nes)
{
    return (nes->ppu.mask >> 2) & 0x01;
}

uint8_t mask_get_bg_on(const struct NES_Core* nes)
{
    return (nes->ppu.mask >> 3) & 0x01;
}

uint8_t mask_get_obj_on(const struct NES_Core* nes)
{
    return (nes->ppu.mask >> 4) & 0x01;
}

uint8_t mask_get_bgr(const struct NES_Core* nes)
{
    return (nes->ppu.mask >> 5) & 0x07;
}

uint8_t status_get_lsb(const struct NES_Core* nes)
{
    return (nes->ppu.status >> 0) & 0x1F;
}

void status_set_obj_overflow(struct NES_Core* nes, uint8_t v)
{
    nes->ppu.status = (nes->ppu.status & ~(0x01 << 5)) | ((v & 0x01) << 5);
}

void status_set_obj_hit(struct NES_Core* nes, uint8_t v)
{
    nes->ppu.status = (nes->ppu.status & ~(0x01 << 6)) | ((v & 0x01) << 6);
}

void status_set_vblank(struct NES_Core* nes, uint8_t v)
{
    nes->ppu.status = (nes->ppu.status & ~(0x01 << 7)) | ((v & 0x01) << 7);
}

static uint16_t ppu_get_nametable_addr(const struct NES_Core* nes)
{
    switch ((nes->ppu.ctrl >> 0) & 0x03)
    {
        case 0: return 0x2000;
        case 1: return 0x2400;
        case 2: return 0x2800;
        case 3: return 0x2C00;
    }

    return 0xFF;
}

static uint16_t ppu_get_bg_pattern_table_addr(const struct NES_Core* nes)
{
    return IS_BIT_SET(nes->ppu.ctrl, 4) * 0x1000;
}

static uint16_t ppu_get_obj_pattern_table_addr(const struct NES_Core* nes)
{
    return IS_BIT_SET(nes->ppu.ctrl, 3) * 0x1000;
}

static uint8_t ppu_get_sprite_size(const struct NES_Core* nes)
{
    return IS_BIT_SET(nes->ppu.ctrl, 5) ? 16 : 8;
}

// nametable is 0x3C0 bytes, then 0x40 bytes of attributes
struct BgAttribute
{
    uint8_t upper_left  /*: 2*/;
    uint8_t upper_right /*: 2*/;
    uint8_t lower_left  /*: 2*/;
    uint8_t lower_right /*: 2*/;
};

// as its 64 bytes next to each other for attr data,
// could use simd to set all the data into the array
static FORCE_INLINE struct BgAttribute gen_bg_attr(const uint8_t v)
{
    return (struct BgAttribute)
    {
        .upper_left   = (v >> 0) & 0x3,
        .upper_right  = (v >> 2) & 0x3,
        .lower_left   = (v >> 4) & 0x3,
        .lower_right  = (v >> 6) & 0x3
    };
}

struct ObjAttribute
{
    bool yflip;
    bool xflip;
    bool bg_prio;
    uint8_t palette;
};

struct Obj
{
    uint8_t y;
    uint8_t n;
    struct ObjAttribute a;
    uint8_t x;

    uint16_t pattern_table_base;
    bool sprite0;
};

struct Sprites
{
    struct Obj sprite[8];
    uint8_t count;
};

static FORCE_INLINE struct ObjAttribute gen_ob_attr(const uint8_t v)
{
    return (struct ObjAttribute)
    {
        .yflip      = (v >> 7) & 0x1,
        .xflip      = (v >> 6) & 0x1,
        .bg_prio    = (v >> 5) & 0x1,
        .palette    = (v >> 0) & 0x3,
    };
}

static struct Sprites sprite_fetch(struct NES_Core* nes)
{
    struct Sprites sprites = {0};
    
    // this is ignored if 8x16
    const uint16_t pattern_table_addr = ppu_get_obj_pattern_table_addr(nes);
    const uint8_t sprite_size = ppu_get_sprite_size(nes);
    const uint8_t ly = nes->ppu.scanline;

    for (size_t i = 0; i < ARRAY_SIZE(nes->ppu.oam); i += 4)
    {
        const uint8_t sprite_y = nes->ppu.oam[i];

        // check if the y is in bounds!
        if (ly >= sprite_y && ly < (sprite_y + sprite_size))
        {
            // only 8 sprites per line!
            if (sprites.count < 8)
            {
                struct Obj* sprite = &sprites.sprite[sprites.count];

                sprite->y = sprite_y;
                sprite->n = nes->ppu.oam[i + 1];
                sprite->a = gen_ob_attr(nes->ppu.oam[i + 2]);
                sprite->x = nes->ppu.oam[i + 3];
                sprite->sprite0 = i == 0;

                if (sprite_size == 8)
                {
                    sprite->pattern_table_base = pattern_table_addr;
                }
                else
                {
                    sprite->pattern_table_base = sprite->n & 0x1 ? 0x1000 : 0x0000;
                    sprite->n &= ~0x1;
                }
                
                ++sprites.count;
            }
            // we have to keep looping through oam in order to check
            // if there were any lost sprites on this line!
            else
            {
                status_set_obj_overflow(nes, true);
                break;
            }
        }
    }

    return sprites;
}

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

uint8_t nes_ppu_read(struct NES_Core* nes, uint16_t addr)
{
    assert(addr <= 0x3FFF);

    if (LIKELY(addr <= 0x3EFF))
    {
        return nes->ppu.map[addr >> 10][addr & 0x3FF];
    }
    else
    {
        addr &= 0x1F;

        if (addr == 0x10 || addr == 0x14 || addr == 0x18 || addr == 0x1C)
        {
            addr -= 0x10;
        }

        return nes->ppu.pram[addr & 0x1F];
    }
}

void nes_ppu_write(struct NES_Core* nes, uint16_t addr, uint8_t value)
{
    assert(addr <= 0x3FFF);

    if (LIKELY(addr <= 0x3EFF))
    {
        nes->ppu.map[addr >> 10][addr & 0x3FF] = value;
    }
    else
    {
        addr &= 0x1F;

        if (addr == 0x10 || addr == 0x14 || addr == 0x18 || addr == 0x1C)
        {
            addr -= 0x10;
        }

        nes->ppu.pram[addr & 0x1F] = value;
    }
}

// this is called by the cpu when writing to $4014 register
void nes_dma(struct NES_Core* nes)
{
    const uint16_t addr = nes->ppu.oam_addr << 8;

    // fills the entire oam!
    for (uint16_t i = 0; i < 0x100; i++)
    {
        nes->ppu.oam[i] = nes_cpu_read(nes, addr | i);
    }
}

// this is kinda slow, but allows but any pixel format to be set.
// this could be a compile-time option instead, with the default being u32
static FORCE_INLINE void ppu_write_pixel(struct NES_Core* nes, uint32_t c, uint16_t x, uint16_t y)
{
    switch (nes->bpp)
    {
        case 8:
            ((uint8_t*)nes->pixels)[nes->pixels_stride * y + x] = c;
            break;

        case 15:
        case 16:
            ((uint16_t*)nes->pixels)[nes->pixels_stride * y + x] = c;
            break;

        case 24:
        case 32:
            ((uint32_t*)nes->pixels)[nes->pixels_stride * y + x] = c;
            break;
    }
}

static FORCE_INLINE uint32_t get_colour_from_palette(struct NES_Core* nes, uint8_t palette, uint8_t colour_id)
{
    const uint8_t offset = (palette * 4) + colour_id;
    const uint8_t index = nes_ppu_read(nes, 0x3F00 | offset);//nes->ppu.pram[offset & 0x1F];
    return nes->palette.colour[index & 0x3F];
}

// same as i used in dmg / gbc / sms
struct PriorityBuf
{
    uint8_t pal[NES_SCREEN_WIDTH];
};

static void render_scanline_bg(struct NES_Core* nes, uint8_t line, struct PriorityBuf* prio)
{
    const uint8_t row = (line >> 3) & 31;
    const uint8_t fine_line = line & 7;

    const uint16_t pattern_table_addr = ppu_get_bg_pattern_table_addr(nes);

    uint16_t nametable_addr = ppu_get_nametable_addr(nes) + (row * 32);
    uint16_t attr_table_addr = ppu_get_nametable_addr(nes) + (row * 4) + 0x3C0;

    // todo: attribute table
    for (uint8_t col = 0; col < 32; ++col)
    {
        const uint8_t tile_num = nes_ppu_read(nes, nametable_addr++);
        const uint8_t attr_byte = nes_ppu_read(nes, attr_table_addr++);
        const struct BgAttribute attr = gen_bg_attr(attr_byte);

        const uint8_t bit_plane0 = nes_ppu_read(nes, pattern_table_addr + fine_line + (tile_num * 16) + 0);
        const uint8_t bit_plane1 = nes_ppu_read(nes, pattern_table_addr + fine_line + (tile_num * 16) + 8);

        for (uint8_t x = 0; x < 8; ++x)
        {
            const uint8_t bit = 7 - x;

            const uint16_t x_index = (col * 8) + x;

            uint8_t palette_index = 0;

            palette_index |= IS_BIT_SET(bit_plane0, bit) << 0;
            palette_index |= IS_BIT_SET(bit_plane1, bit) << 1;

            prio->pal[x_index] = palette_index;

            (void)attr;
            const uint32_t colour = get_colour_from_palette(nes, 3, palette_index);

            ppu_write_pixel(nes, colour, x_index, line);
        }
    }
}

static void render_scanline_obj(struct NES_Core* nes, uint8_t line, const struct PriorityBuf* prio)
{
    const struct Sprites sprites = sprite_fetch(nes);
    const uint8_t sprite_size = ppu_get_sprite_size(nes);

    bool already_rendered[NES_SCREEN_WIDTH] = {0};

    for (uint8_t i = 0; i < sprites.count; ++i)
    {
        const struct Obj* sprite = &sprites.sprite[i];
    
        uint16_t pattern_index = sprite->pattern_table_base + (sprite->n * 16);
        
        // check if the sprite is upside down
        if (sprite->a.yflip)
        {
            pattern_index += 7 - ((line - sprite->y) & 0x7);
        
            // check if we are on the next row
            if (sprite_size == 16 && (line - sprite->y) < 8)
            {
                pattern_index += 16;
            }
        }
        else
        {
            pattern_index += ((line - sprite->y) & 0x7);
        
            // check if we are on the next row
            if (sprite_size == 16 && (line - sprite->y) >= 8)
            {
                pattern_index += 16;
            }
        }

        const uint8_t bit_plane0 = nes_ppu_read(nes, pattern_index + 0);
        const uint8_t bit_plane1 = nes_ppu_read(nes, pattern_index + 8);

        for (uint8_t x = 0; x < 8; ++x)
        {
            const uint8_t bit = sprite->a.xflip ? x : 7 - x;

            const uint16_t x_index = sprite->x + x;

            if (x_index > NES_SCREEN_WIDTH)
            {
                break;
            }

            uint8_t palette_index = 0;

            palette_index |= IS_BIT_SET(bit_plane0, bit) << 0;
            palette_index |= IS_BIT_SET(bit_plane1, bit) << 1;

            // transparent
            if (palette_index == 0)
            {
                continue;
            }

            // set if oam[0] is being rendered over pal 1-3 bg.
            // it does not care for bg priority!
            if (sprite->sprite0 && prio->pal[x_index] != 0)
            {
                status_set_obj_hit(nes, true);
            }

            // skip if sprite has already been rendered
            if (already_rendered[x_index])
            {
                continue;
            }

            // keep track of the sprite already being rendered
            // this is set regardless of bg priority!
            already_rendered[x_index] = true;

            // skip if sprite is behind bg AND bg is transparent
            if (sprite->a.bg_prio && prio->pal[x_index] != 0)
            {
                continue;
            }

            const uint32_t colour = get_colour_from_palette(nes, 4 + sprite->a.palette, palette_index);

            ppu_write_pixel(nes, colour, x_index, line);
        }
    }
}

static void render_scanline(struct NES_Core* nes, int line)
{
    if (line >= 240 || line < 0)
    {
        return;
    }

    struct PriorityBuf prio = {0};

    if (mask_get_bg_on(nes))
    {
        render_scanline_bg(nes, line, &prio);
    }

    if (mask_get_obj_on(nes))
    {
        render_scanline_obj(nes, line, &prio);
    }
}

// there are 262 scanlines total
// each scanline takes 341 ppu clock, so ~113 cpu clocks
// a pixel is created every clock cycle (ppu cycle?)
void nes_ppu_run(struct NES_Core* nes, const uint16_t cycles_elapsed)
{
    nes->ppu.cycles += cycles_elapsed * 3;

    if (UNLIKELY(nes->ppu.cycles >= 341))
    {
        if (nes->pixels)
        {
            render_scanline(nes, nes->ppu.scanline);
        }
 
        // hack to get games working
        if (nes->ppu.scanline == 80)
        {
            status_set_obj_hit(nes, 1);
        }

        nes->ppu.cycles -= 341;
        ++nes->ppu.scanline;

        // vblank
        if (nes->ppu.scanline == 240)
        {
            if (nes->vblank_callback)
            {
                nes->vblank_callback(nes->vblank_callback_user);
            }

            // set the status to vblank
            status_set_vblank(nes, true);

            // check if the nmi bit is set, if so then fire an nmi.
            if (ctrl_get_nmi(nes))
            {
                nes_cpu_nmi(nes);
            }
        }

        // reset
        else if (nes->ppu.scanline == 261)
        {
            // reset status
            status_set_vblank(nes, false);
            status_set_obj_overflow(nes, false);
            status_set_obj_hit(nes, false);
            nes->ppu.scanline = -1;
        }
    }
}

void nes_ppu_init(struct NES_Core* nes)
{
    (void)nes;
}
