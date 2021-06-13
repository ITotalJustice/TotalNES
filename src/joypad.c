#include "nes.h"
#include "internal.h"

#include <assert.h>


#define JOYPAD nes->jp


uint8_t nes_joypad_read_port_0(struct NES_Core* nes)
{
    // i think this returns 1 if connected?
    if (JOYPAD.strobe)
    {
        return 1;
    }

    // read 1-bit MSB first
    const uint8_t button_bit = (JOYPAD.latch_a & 0x80) != 0;

    // shift to the next bit
    JOYPAD.latch_a <<= 0x01;

    // reads after bit-8 return 1's, we can simulate this by shifting in
    // a one after each left-shift.
    // 0b0000'1000 -> 0b0001'0001 -> 0b0010'0011 -> 0b0100'0111
    JOYPAD.latch_a |= 0x01;

    // OR with unsued bits!
    return button_bit | 0x40;
}


// [API]
void nes_set_button(struct NES_Core* nes, const enum NES_Button button, const bool down)
{
    if (down)
    {
        nes->jp.buttons_a |= button;
    }
    else
    {
        nes->jp.buttons_a &= ~button;
    }
}

