#include "nes.h"
#include "internal.h"

#include <assert.h>


#define JOYPAD nes->jp


uint8_t nes_joypad_read_port_0(struct NES_Core* nes)
{
    if (JOYPAD.strobe)
    {
        // return 0x1;
        // JOYPAD.latch_a = JOYPAD.buttons_a;
        // unused
        // JOYPAD.latch_b = JOYPAD.buttons_b;
    }

    // read 1-bit LSB first
    const bool button_bit = JOYPAD.latch_a & 0x1;

    // shift to the next bit
    JOYPAD.latch_a >>= 0x01;

    // OR with unsued bits!
    return button_bit | 0x40;
}

void nes_joypad_write(struct NES_Core* nes, uint8_t value)
{
    JOYPAD.strobe = value & 0x1;

    if (JOYPAD.strobe)
    {
        JOYPAD.latch_a = JOYPAD.buttons_a;
        // unused
        JOYPAD.latch_b = JOYPAD.buttons_b;
    }
}

// [API]
void NES_set_button(struct NES_Core* nes, enum NES_Button button, bool down)
{
    if (down)
    {
        JOYPAD.buttons_a |= button;
    }
    else
    {
        JOYPAD.buttons_a &= ~button;
    }
}

