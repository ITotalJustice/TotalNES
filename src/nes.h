#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"


struct NES_State;


// DK starts with the [START] button being in this order
// but im not sure if it's correct, it doesn't seem to work
// when in game either
enum NES_Button
{
    NES_BUTTON_A      = (1 << 7),
    NES_BUTTON_B      = (1 << 6),
    NES_BUTTON_SELECT = (1 << 5),
    NES_BUTTON_START  = (1 << 4),

    NES_BUTTON_UP     = (1 << 3),
    NES_BUTTON_DOWN   = (1 << 2),
    NES_BUTTON_LEFT   = (1 << 1),
    NES_BUTTON_RIGHT  = (1 << 0),
};


// API
void nes_reset(struct NES_Core* nes);

bool nes_is_header_valid(const struct NES_CartHeader* header);

bool nes_loadrom(struct NES_Core* nes, uint8_t* buffer, size_t size);

void nes_run_frame(struct NES_Core* nes);

void nes_set_button(struct NES_Core* nes, enum NES_Button button, bool down);

void nes_set_apu_callback(struct NES_Core* nes, NES_apu_callback_t cb);

#ifdef __cplusplus
}
#endif