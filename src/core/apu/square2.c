#include "core/nes.h"
#include "core/internal.h"
#include "core/apu/apu.h"

#include <assert.h>


bool is_square2_enabled(const struct NES_Core* nes) {
    return STATUS.square2_enable;
}

bool is_square2_length_enabled(const struct NES_Core* nes) {
    return SQUARE2_CHANNEL.length_counter_halt == 0;// && APU.status.pulse1 == 0;
}

bool is_square2_sweep_enabled(const struct NES_Core* nes) {
    return SQUARE2_CHANNEL.sweep_enabled > 0;
}

uint16_t get_square2_freq(const struct NES_Core* nes) {
    return SQUARE2_CHANNEL.timer_reload;
}

void clock_square2_length(struct NES_Core* nes) {
    // check if length counting is enabled!
    if (!is_square2_length_enabled(nes)) {
        return;
    }

    if (SQUARE2_CHANNEL.length_counter > 0) {
        // otherwise we clock it
        --SQUARE2_CHANNEL.length_counter;
    }
}

void clock_square2_envelope(struct NES_Core* nes) {
    if (SQUARE2_CHANNEL.constant_volume == 1) {
        return;
    }

    if (SQUARE2_CHANNEL.envelope_counter >= 0) {
        --SQUARE2_CHANNEL.envelope_counter;

        if (SQUARE2_CHANNEL.envelope_counter <= 0) {
            // reload!
            SQUARE2_CHANNEL.envelope_counter = SQUARE2_CHANNEL.volume + 1;

            if (SQUARE2_CHANNEL.volume == 0) {
                // check if we loop!
                if (SQUARE2_CHANNEL.length_counter_halt == 1) {
                    SQUARE2_CHANNEL.volume = 15;
                }
            }
            else {
                // decrement like normal
                --SQUARE2_CHANNEL.volume;
            }
        }
    }
}

/*
    If the current period is less than 8, the sweep unit mutes the channel.
    If at any time the target period is greater than $7FF, the sweep unit mutes the channel.
*/
void clock_square2_sweep(struct NES_Core* nes) {
    if (!is_square2_sweep_enabled(nes)) {
        return;
    }

    if (SQUARE2_CHANNEL.sweep_counter >= 0) {
        --SQUARE2_CHANNEL.sweep_counter;

        if (SQUARE2_CHANNEL.sweep_counter <= 0) {
            SQUARE2_CHANNEL.sweep_counter = SQUARE2_CHANNEL.sweep_period;

            const uint32_t old_freq = get_square2_freq(nes);
            int32_t difference = old_freq >> SQUARE2_CHANNEL.sweep_shift;

            // check if we need to make the result negative
            if (SQUARE2_CHANNEL.sweep_negate) {
                difference = -difference;
            }

            const int32_t new_freq = old_freq + difference;

            // disable
            if (new_freq < 8 || new_freq > 0x7FF) {
                SQUARE2_CHANNEL.volume = 0;
            }
            else {
                // otherwise set the new value!
                SQUARE2_CHANNEL.timer_reload = new_freq;
            }
        }
    }
}

void clock_square2_duty(struct NES_Core* nes) {
    ++SQUARE2_CHANNEL.duty_index;
}

int8_t sample_square2(const struct NES_Core* nes) {
    if (SQUARE2_CHANNEL.length_counter == 0) {
        return -SQUARE2_CHANNEL.volume;
    }

    if (SQUARE_DUTY[SQUARE2_CHANNEL.duty][SQUARE2_CHANNEL.duty_index]) {
        return SQUARE2_CHANNEL.volume;
    }

    return -SQUARE2_CHANNEL.volume;
}
