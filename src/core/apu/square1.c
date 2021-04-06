#include "core/nes.h"
#include "core/internal.h"
#include "core/apu/apu.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>


bool is_square1_enabled(const struct NES_Core* nes) {
    return SQUARE1_CHANNEL.length_counter > 0;
}

bool is_square1_length_enabled(const struct NES_Core* nes) {
    return SQUARE1_CHANNEL.length_counter_halt == 0;// && APU.status.pulse1 == 0;
}

bool is_square1_sweep_enabled(const struct NES_Core* nes) {
    return SQUARE1_CHANNEL.sweep_enabled > 0;
}

uint16_t get_square1_freq(const struct NES_Core* nes) {
    return SQUARE1_CHANNEL.freq;
}

void clock_square1_length(struct NES_Core* nes) {
    // check if length counting is enabled!
    if (!is_square1_length_enabled(nes)) {
        return;
    }

    if (SQUARE1_CHANNEL.length_counter > 0) {
        // otherwise we clock it
        --SQUARE1_CHANNEL.length_counter;

        // check if we are zero!
    }
}

void clock_square1_envelope(struct NES_Core* nes) {
    if (SQUARE1_CHANNEL.constant_volume == 1) {
        return;
    }

    if (SQUARE1_CHANNEL.envelope_counter >= 0) {
        --SQUARE1_CHANNEL.envelope_counter;

        if (SQUARE1_CHANNEL.envelope_counter <= 0) {
            // reload!
            SQUARE1_CHANNEL.envelope_counter = SQUARE1_CHANNEL.volume + 1;

            if (SQUARE1_CHANNEL.volume == 0) {
                // check if we loop!
                if (SQUARE1_CHANNEL.length_counter_halt == 1) {
                    SQUARE1_CHANNEL.volume = 15;
                }
            }
            else {
                // decrement like normal
                --SQUARE1_CHANNEL.volume;
            }
        }
    }
}

/*
    If the current period is less than 8, the sweep unit mutes the channel.
    If at any time the target period is greater than $7FF, the sweep unit mutes the channel.
*/
void clock_square1_sweep(struct NES_Core* nes) {
    if (!is_square1_sweep_enabled(nes)) {
        return;
    }

    if (SQUARE1_CHANNEL.sweep_counter >= 0) {
        --SQUARE1_CHANNEL.sweep_counter;

        if (SQUARE1_CHANNEL.sweep_counter <= 0) {
            SQUARE1_CHANNEL.sweep_counter = SQUARE1_CHANNEL.sweep_period;

            const uint32_t old_freq = get_square1_freq(nes);
            int32_t difference = old_freq >> SQUARE1_CHANNEL.sweep_shift;

            // check if we need to make the result negative
            if (SQUARE1_CHANNEL.sweep_negate) {
                difference = -difference;
            }

            const int32_t new_freq = old_freq + difference;

            // disable
            if (new_freq < 8 || new_freq > 0x7FF) {
                SQUARE1_CHANNEL.volume = 0;
            }
            else {
                // otherwise set the new value!
                SQUARE1_CHANNEL.freq = new_freq;
            }
        }
    }
}

int8_t sample_square1(const struct NES_Core* nes) {
    if (SQUARE_DUTY[SQUARE1_CHANNEL.duty][SQUARE1_CHANNEL.duty_index]) {
        return SQUARE1_CHANNEL.volume;
    }

    return -SQUARE1_CHANNEL.volume;
}
