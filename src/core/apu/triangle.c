#include "core/nes.h"
#include "core/internal.h"
#include "core/apu/apu.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>


bool is_triangle_enabled(const struct NES_Core* nes) {
    return TRIANGLE_CHANNEL.length_counter > 0;
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

        // check if we are zero!
    }
}

void clock_triangle_linear(struct NES_Core* nes) {

}

int8_t sample_triangle(const struct NES_Core* nes) {
    // i think this is right???
    return TRIANGLE_DUTY_TABLE[TRIANGLE_CHANNEL.duty_index] * 2;
}


