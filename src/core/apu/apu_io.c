#include "core/nes.h"
#include "core/internal.h"
#include "core/apu/apu.h"


#include <stdio.h>
#include <assert.h>


uint8_t NES_apu_io_read(struct NES_Core* nes, const uint16_t addr) {
    uint8_t data = 0xFF;

    switch (addr & 0x1F) {
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
        case 0x10: case 0x11: case 0x12: case 0x13:
            // reading from these registers
            // returns the last written value, NOT the current
            // value stored!
            data = APU.io[addr & 0x1F];
            break;

        case 0x15:
            data = nes->apu.io[addr & 0x1F] & ~(0xF);
            data |= is_square1_length_enabled(nes) << 0;
            // data |= is_square2_length_enabled(nes) << 1;
            data |= 0x04;//is_triangle_length_enabled(nes) << 0;
            data |= 0x08;//is_pulse1_length_enabled(nes) << 0;
            nes->apu.status.frame_irq = 0;
            break;

    }

    return data;
}


void NES_apu_io_write(struct NES_Core* nes, const uint16_t addr, const uint8_t value) {
    nes->apu.io[addr & 0x1F] = value;

    switch (addr & 0x1F) {
    // [SQAURE1]
        case 0x00:
            SQUARE1_CHANNEL.duty = value >> 6;
            SQUARE1_CHANNEL.length_counter_halt = (value >> 5) & 1;
            SQUARE1_CHANNEL.constant_volume = (value >> 4) & 1;
            SQUARE1_CHANNEL.volume = value & 0x0F;
            break;

        case 0x01:
            SQUARE1_CHANNEL.sweep_enabled = value >> 7;
            SQUARE1_CHANNEL.sweep_period = (value >> 4) & 0x7;
            SQUARE1_CHANNEL.sweep_negate = value >> 3;
            SQUARE1_CHANNEL.sweep_shift = value & 0x7;
            break;

        case 0x02:
            SQUARE1_CHANNEL.timer_reload = (SQUARE1_CHANNEL.timer_reload & 0xFF00) | value;
            break;

        case 0x03:
            SQUARE1_CHANNEL.timer_reload = (SQUARE1_CHANNEL.timer_reload & 0xFF) | ((value & 0x7) << 8);
            SQUARE1_CHANNEL.length_counter = LENGTH_COUNTER_TABLE[(value >> 3)];
            SQUARE1_CHANNEL.envelope_counter = SQUARE1_CHANNEL.volume + 1;
            break;

    // [SQAURE2]
        case 0x04:
            SQUARE2_CHANNEL.duty = value >> 6;
            SQUARE2_CHANNEL.length_counter_halt = (value >> 5) & 1;
            SQUARE2_CHANNEL.constant_volume = (value >> 4) & 1;
            SQUARE2_CHANNEL.volume = value & 0x0F;
            break;

        case 0x05:
            SQUARE2_CHANNEL.sweep_enabled = value >> 7;
            SQUARE2_CHANNEL.sweep_period = (value >> 4) & 0x7;
            SQUARE2_CHANNEL.sweep_negate = value >> 3;
            SQUARE2_CHANNEL.sweep_shift = value & 0x7;
            break;

        case 0x06:
            SQUARE2_CHANNEL.timer_reload = (SQUARE2_CHANNEL.timer_reload & 0xFF00) | value;
            break;

        case 0x07:
            SQUARE2_CHANNEL.timer_reload = (SQUARE2_CHANNEL.timer_reload & 0xFF) | ((value & 0x7) << 8);
            SQUARE2_CHANNEL.length_counter = LENGTH_COUNTER_TABLE[(value >> 3)];
            SQUARE2_CHANNEL.envelope_counter = SQUARE2_CHANNEL.volume + 1;
            break;

    // [TRIANGLE]
        case 0x08:
            break;

        case 0x0A:
            TRIANGLE_CHANNEL.timer_reload = (TRIANGLE_CHANNEL.timer_reload & 0xFF00) | value;
            break;

        case 0x0B:
            TRIANGLE_CHANNEL.timer_reload = (TRIANGLE_CHANNEL.timer_reload & 0xFF) | ((value & 0x7) << 8);
            TRIANGLE_CHANNEL.length_counter = LENGTH_COUNTER_TABLE[(value >> 3)];
            break;

    // [NOISE]
        case 0x0C:
            NOISE_CHANNEL.length_counter_halt = (value >> 5) & 1;
            NOISE_CHANNEL.constant_volume = (value >> 4) & 1;
            NOISE_CHANNEL.volume = value & 0x0F;
            break;

        case 0x0E:
            NOISE_CHANNEL.random_number_mode = value >> 7;
            NOISE_CHANNEL.frequency = value & 0x7;
            break;

        case 0x0F:
            NOISE_CHANNEL.length_counter = LENGTH_COUNTER_TABLE[(value >> 3)];
            NOISE_CHANNEL.envelope_counter = NOISE_CHANNEL.volume + 1;
            break;

    // [DMC]
        case 0x10:
            break;

        case 0x11:
            break;

        case 0x12:
            break;

        case 0x13:
            break;

    // [STATUS]
        case 0x15:
            STATUS.square1_enable = value & 1;
            STATUS.square2_enable = (value >> 1) & 1;
            STATUS.triangle_enable = (value >> 2) & 1;
            STATUS.noise_enable = (value >> 3) & 1;
            STATUS.dmc_enable = (value >> 4) & 1;
            break;

    // [FRAME-SEQUENCER]
        case 0x17:
            // this resets the frame counter and clock divider(?)
            FRAME_SEQUENCER.step = 0;
            FRAME_SEQUENCER.mode = value >> 7;
            FRAME_SEQUENCER.irq_enable = (value >> 6) & 1;
            break;
    }
}
