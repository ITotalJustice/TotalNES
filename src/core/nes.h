#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"


struct NES_State;


// DK starts with the [START] button being in this order
// but im not sure if it's correct, it doesn't seem to work
// when in game either
enum NES_Button {
    NES_BUTTON_A        = (1 << 7),
    NES_BUTTON_B        = (1 << 6),
    NES_BUTTON_SELECT   = (1 << 5),
    NES_BUTTON_START    = (1 << 4),

    NES_BUTTON_UP       = (1 << 3),
    NES_BUTTON_DOWN     = (1 << 2),
    NES_BUTTON_LEFT     = (1 << 1),
    NES_BUTTON_RIGHT    = (1 << 0),
};


// API
void NES_reset(struct NES_Core* nes);

bool NES_is_header_valid(struct NES_Core* nes,
	const struct NES_CartHeader* header
);

bool NES_loadrom(struct NES_Core* nes,
	uint8_t* buffer, size_t size
);

void NES_run_frame(struct NES_Core* nes);

void NES_set_button(struct NES_Core* nes,
	const enum NES_Button button, const bool down
);

void NES_set_apu_callback(struct NES_Core* nes,
	NES_apu_callback_t cb, void* user_data
);

// NOT DONE
bool NES_savestate(struct NES_Core* nes,
	struct NES_State* state
);

bool NES_loadstate(struct NES_Core* nes,
	const struct NES_State* state
);

#ifdef __cplusplus
}
#endif
