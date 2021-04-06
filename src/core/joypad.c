#include "core/nes.h"
#include "core/internal.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>


uint8_t NES_joypad_read_port_0(struct NES_Core* nes) {
    if (nes->jp.shift <= 7) {
        const uint8_t result = (nes->jp.buttons & (1 << nes->jp.shift)) > 0;
        ++nes->jp.shift;

        return result;
    }
    else {
        return 0xFF;
    }
}


// [API]
void NES_set_button(struct NES_Core* nes, const enum NES_Button button, const bool down) {
    if (down) {
        nes->jp.buttons |= button;
    } else {
        nes->jp.buttons &= ~button;
    }
}

