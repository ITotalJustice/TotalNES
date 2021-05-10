#include "../nes.h"
#include "../internal.h"
#include "apu.h"

#include <assert.h>


bool is_triangle_enabled(const struct NES_Core* nes) {
    return STATUS.triangle_enable;
}

bool is_triangle_length_enabled(const struct NES_Core* nes) {
    return TRIANGLE_CHANNEL.length_counter_halt == 0;
}

uint16_t get_triangle_freq(const struct NES_Core* nes) {
    return TRIANGLE_CHANNEL.timer_reload;
}

void clock_triangle_length(struct NES_Core* nes) {
    // check if length counting is enabled!
    if (!is_triangle_length_enabled(nes)) {
        return;
    }

    if (TRIANGLE_CHANNEL.length_counter > 0) {
        // otherwise we clock it
        --TRIANGLE_CHANNEL.length_counter;
    }
}

void clock_triangle_linear(struct NES_Core* nes) {
    if (!is_triangle_length_enabled(nes)) {
        return;
    }

    if (TRIANGLE_CHANNEL.linear_counter_load > 0) {
        // otherwise we clock it
        --TRIANGLE_CHANNEL.linear_counter_load;
    }
}

void clock_triangle_duty(struct NES_Core* nes) {
    ++TRIANGLE_CHANNEL.duty_index;
    TRIANGLE_CHANNEL.duty_index &= 0x1F;
}

int8_t sample_triangle(const struct NES_Core* nes) {
    if (TRIANGLE_CHANNEL.length_counter == 0 || TRIANGLE_CHANNEL.linear_counter_load == 0) {
        return 0;
    }

    // i think this is right???
    return TRIANGLE_DUTY_TABLE[TRIANGLE_CHANNEL.duty_index] * 2;
}


