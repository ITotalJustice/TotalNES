#include "core/nes.h"
#include "core/internal.h"
#include "core/apu/apu.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>


enum {
    SAMPLE_RATE = NES_CPU_CYCLES / 48000,
};


const bool SQUARE_DUTY[4][8] = {
    [0] = { 0, 1, 0, 0, 0, 0, 0, 0 }, // (12.5%)
    [1] = { 0, 1, 1, 0, 0, 0, 0, 0 }, // (25%)
    [2] = { 0, 1, 1, 1, 1, 0, 0, 0 }, // (50%)
    [3] = { 1, 0, 0, 1, 1, 1, 1, 1 }, // (25% negated)
};

const uint8_t LENGTH_COUNTER_TABLE[0x20] = {
    0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
    0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
    0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
    0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E,
};

const uint16_t NOISE_TIMER_TABLE[0x10] = {
    0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0A0,
    0x0CA, 0x0FE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4,
};


static void on_clock_irq(struct NES_Core* nes) {
    // todo: fire actual IRQ
    if (FRAME_SEQUENCER.irq_enable) {
        STATUS.frame_irq = 1;
    }
}

static void on_clock_length_and_sweep(struct NES_Core* nes) {
    clock_square1_length(nes);
    clock_square2_length(nes);
    clock_square1_sweep(nes);
    clock_square2_sweep(nes);
}

static void on_clock_envelope_and_linear_counter(struct NES_Core* nes) {
    // todo:
    clock_square1_envelope(nes);
    clock_square2_envelope(nes);
}

static void frame_sequencer_clock(struct NES_Core* nes) {
    // 2-modes, 4-step vs 5-step
    enum { MODE_4 = 0, MODE_5 = 1 };

    if (APU.frame_sequencer.mode == MODE_4) {
        switch (APU.frame_sequencer.step) {
            case 0:
                on_clock_envelope_and_linear_counter(nes);
                break;

            case 1:
                on_clock_length_and_sweep(nes);
                on_clock_envelope_and_linear_counter(nes);
                break;

            case 2:
                on_clock_envelope_and_linear_counter(nes);
                break;

            case 3:
                on_clock_irq(nes);
                break;
        }

        // advance the step, wrapping around if needed!
        APU.frame_sequencer.step = (APU.frame_sequencer.step + 1) % 4;
    }

    else {
        switch (APU.frame_sequencer.step) {
            case 0:
                on_clock_envelope_and_linear_counter(nes);
                break;

            case 1:
                on_clock_length_and_sweep(nes);
                on_clock_envelope_and_linear_counter(nes);
                break;

            case 2:
                on_clock_envelope_and_linear_counter(nes);
                break;

            case 3:
                break;

            case 4:
                on_clock_length_and_sweep(nes);
                on_clock_envelope_and_linear_counter(nes);
                break;
        }

        // advance the step, wrapping around if needed!
        APU.frame_sequencer.step = (APU.frame_sequencer.step + 1) % 5;
    }
}

static void sample(struct NES_Core* nes) {
    const int8_t square1_sample = sample_square1(nes);// * is_square1_enabled(nes);
    const int8_t square2_sample = sample_square2(nes);// * is_square2_enabled(nes);

#if 0
    const int8_t final = square1_sample;
#else
    const int8_t final = ((square1_sample) + (square2_sample)) / 2;
#endif

    APU.sample_data.samples[APU.sample_index] = final;

    // mono
    ++APU.sample_index;

    if (APU.sample_index >= NES_ARR_SIZE(APU.sample_data.samples)) {
        APU.sample_index = 0;

        if (nes->apu_cb != NULL) {
            nes->apu_cb(nes, nes->apu_cb_user_data, &APU.sample_data);
        }
    }
}

void NES_apu_run(struct NES_Core* nes, const uint16_t cycles_elapsed) {
    // atm im multiplying the freq by 2, it sounds too high pitch at normal
    // frequency for some reason...
    // i read that its 2 cpu cycles for 1 apu cycle, so maybe thats why?

    SQUARE1_CHANNEL.timer -= cycles_elapsed;
    if (SQUARE1_CHANNEL.timer <= 0) {
        SQUARE1_CHANNEL.timer += get_square1_freq(nes) << 1;
        SQUARE1_CHANNEL.duty_index++;
    }

    SQUARE2_CHANNEL.timer -= cycles_elapsed;
    if (SQUARE2_CHANNEL.timer <= 0) {
        SQUARE2_CHANNEL.timer += get_square2_freq(nes) << 1;
        SQUARE2_CHANNEL.duty_index++;
    }

    APU.frame_sequencer.timer -= cycles_elapsed;
    while (APU.frame_sequencer.timer <= 0) {
        APU.frame_sequencer.timer += NES_APU_FRAME_SEQUENCER_STEP_RATE << 1;
        frame_sequencer_clock(nes);
    }

    APU.sample_cycles -= cycles_elapsed;
    while (APU.sample_cycles <= 0) {
        APU.sample_cycles += SAMPLE_RATE;
        sample(nes);
    }
}
