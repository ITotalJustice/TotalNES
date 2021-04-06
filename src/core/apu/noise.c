#include "core/nes.h"
#include "core/internal.h"
#include "core/apu/apu.h"

#include <assert.h>


bool is_noise_enabled(const struct NES_Core* nes) {
    return STATUS.noise_enable;
}

bool is_noise_length_enabled(const struct NES_Core* nes) {
    return NOISE_CHANNEL.length_counter_halt == 0;
}

uint16_t get_noise_freq(const struct NES_Core* nes) {
    return NOISE_TIMER_TABLE[NOISE_CHANNEL.frequency];
}

void clock_noise_length(struct NES_Core* nes) {
    // check if length counting is enabled!
    if (!is_noise_length_enabled(nes)) {
        return;
    }

    if (NOISE_CHANNEL.length_counter > 0) {
        // otherwise we clock it
        --NOISE_CHANNEL.length_counter;
    }
}

void clock_noise_envelope(struct NES_Core* nes) {
    if (NOISE_CHANNEL.constant_volume == 1) {
        return;
    }

    if (NOISE_CHANNEL.envelope_counter >= 0) {
        --NOISE_CHANNEL.envelope_counter;

        if (NOISE_CHANNEL.envelope_counter <= 0) {
            // reload!
            NOISE_CHANNEL.envelope_counter = NOISE_CHANNEL.volume + 1;

            if (NOISE_CHANNEL.volume == 0) {
                // check if we loop!
                if (NOISE_CHANNEL.length_counter_halt == 1) {
                    NOISE_CHANNEL.volume = 15;
                }
            }
            else {
                // decrement like normal
                --NOISE_CHANNEL.volume;
            }
        }
    }
}

void clock_noise_lsfr(struct NES_Core* nes) {
    enum { BIT6 = 1 };

    const uint8_t bit0 = NOISE_CHANNEL.lsfr & 0x1;
    uint8_t bitx;

    // if the mode is set, we use bit-6 as the second bit
    // else we use bit-1
    if (NOISE_CHANNEL.random_number_mode == BIT6) {
        bitx = (NOISE_CHANNEL.lsfr >> 6) & 0x1;
    } else {
        bitx = (NOISE_CHANNEL.lsfr >> 1) & 0x1;
    }

    const uint8_t ord_bit = bit0 ^ bitx;

    // shift down te lsfr
    NOISE_CHANNEL.lsfr >>= 1;

    // set the or'd bit to bit-14
    NOISE_CHANNEL.lsfr |= ord_bit << 14;
}

int8_t sample_noise(const struct NES_Core* nes) {
    if (NOISE_CHANNEL.length_counter == 0) {
        return -NOISE_CHANNEL.volume;
    }

    // this is actually the inverse.
    // so only play a sound if bit-0 is *clear*
    // this is the same as the gameboy!
    if ((NOISE_CHANNEL.lsfr & 0x1) == 0) {
        return NOISE_CHANNEL.volume;
    }

    return -NOISE_CHANNEL.volume;
}


