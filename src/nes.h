#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"


enum NES_Button
{
    NES_BUTTON_A      = (1 << 0),
    NES_BUTTON_B      = (1 << 1),
    NES_BUTTON_SELECT = (1 << 2),
    NES_BUTTON_START  = (1 << 3),

    NES_BUTTON_UP     = (1 << 4),
    NES_BUTTON_DOWN   = (1 << 5),
    NES_BUTTON_LEFT   = (1 << 6),
    NES_BUTTON_RIGHT  = (1 << 7),
};

// API
NESAPI bool NES_init(struct NES_Core* nes);
// void NES_reset(struct NES_Core* nes);

// NESAPI bool NES_is_header_valid(const struct NES_CartHeader* header);

NESAPI void NES_set_pixels(struct NES_Core* nes, void* pixels, uint32_t stride, uint8_t bpp);
NESAPI void NES_set_palette(struct NES_Core* nes, const struct NES_Palette* palette);

NESAPI bool NES_loadrom(struct NES_Core* nes, const uint8_t* rom, size_t size);

NESAPI void NES_run_step(struct NES_Core* nes);
NESAPI void NES_run_frame(struct NES_Core* nes);

NESAPI void NES_set_button(struct NES_Core* nes, enum NES_Button button, bool down);

NESAPI void NES_set_apu_callback(struct NES_Core* nes, nes_apu_callback_t cb, void* user, uint32_t freq);
NESAPI void NES_set_vblank_callback(struct NES_Core* nes, nes_vblank_callback_t cb, void* user);

#ifdef __cplusplus
}
#endif
