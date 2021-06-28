#include "../nes.h"
#include "../internal.h"
#include "apu.h"

#include <assert.h>


const bool SQUARE_DUTY[4][8] =
{
    [0] = { 0, 1, 0, 0, 0, 0, 0, 0 }, // (12.5%)
    [1] = { 0, 1, 1, 0, 0, 0, 0, 0 }, // (25%)
    [2] = { 0, 1, 1, 1, 1, 0, 0, 0 }, // (50%)
    [3] = { 1, 0, 0, 1, 1, 1, 1, 1 }, // (25% negated)
};

const uint8_t LENGTH_COUNTER_TABLE[0x20] =
{
    0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
    0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
    0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
    0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E,
};

const uint16_t NOISE_TIMER_TABLE[0x10] =
{
    0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0A0,
    0x0CA, 0x0FE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4,
};

// this is the volume for the triangle
// the table isn't accurate yet, just oprox, need to correct it more
// for signed values
const int8_t TRIANGLE_DUTY_TABLE[0x20] =
{
    -7, -6, -5, -4, -3, -2, -1, -0,
    +1, +2, +3, +4, +5, +6, +7, +8,
    +8, +7, +6, +5, +4, +3, +2, +1,
    +0, -1, -2, -3, -4, -5, -6, -7
};

static FORCE_INLINE void on_clock_irq(struct NES_Core* nes)
{
    // todo: fire actual IRQ
    if (FRAME_SEQUENCER.irq_enable)
    {
        STATUS.frame_irq = 1;
    }
}

static FORCE_INLINE void on_clock_length_and_sweep(struct NES_Core* nes)
{
    clock_square1_length(nes);
    clock_square2_length(nes);
    clock_triangle_length(nes);
    clock_noise_length(nes);

    clock_square1_sweep(nes);
    clock_square2_sweep(nes);
}

static FORCE_INLINE void on_clock_envelope_and_linear_counter(struct NES_Core* nes)
{
    clock_square1_envelope(nes);
    clock_square2_envelope(nes);
    clock_noise_envelope(nes);

    clock_triangle_linear(nes);
}

static FORCE_INLINE void frame_sequencer_clock(struct NES_Core* nes)
{
    // 2-modes, 4-step vs 5-step
    enum { MODE_4 = 0, MODE_5 = 1 };

    if (APU.frame_sequencer.mode == MODE_4)
    {
        switch (APU.frame_sequencer.step)
        {
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

    else
    {
        switch (APU.frame_sequencer.step)
        {
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

static FORCE_INLINE void sample(struct NES_Core* nes)
{
    struct NES_ApuCallbackData data =
    {
        .square1    = sample_square1(nes)   * is_square1_enabled(nes),
        .square2    = sample_square2(nes)   * is_square2_enabled(nes),
        .triangle   = sample_triangle(nes)  * is_triangle_enabled(nes),
        .noise      = sample_noise(nes)     * is_noise_enabled(nes),
    };

    nes->apu_callback(nes->apu_callback_user, &data);
}

void nes_apu_run(struct NES_Core* nes, const uint16_t cycles_elapsed)
{
    if (SQUARE1_CHANNEL.timer > 0 || get_square1_freq(nes))
    {
        SQUARE1_CHANNEL.timer -= cycles_elapsed;
        if (SQUARE1_CHANNEL.timer <= 0)
        {
            SQUARE1_CHANNEL.timer += get_square1_freq(nes) << 1;
            clock_square1_duty(nes);
        }
    }

    if (SQUARE2_CHANNEL.timer > 0 || get_square2_freq(nes))
    {
        SQUARE2_CHANNEL.timer -= cycles_elapsed;
        if (SQUARE2_CHANNEL.timer <= 0)
        {
            SQUARE2_CHANNEL.timer += get_square2_freq(nes) << 1;
            clock_square2_duty(nes);
        }
    }

    if (TRIANGLE_CHANNEL.timer > 0 || get_triangle_freq(nes))
    {
        TRIANGLE_CHANNEL.timer -= cycles_elapsed;
        if (TRIANGLE_CHANNEL.timer <= 0)
        {
            TRIANGLE_CHANNEL.timer += get_triangle_freq(nes);
            clock_triangle_duty(nes);
        }
    }

    if (NOISE_CHANNEL.timer > 0 || get_noise_freq(nes))
    {
        NOISE_CHANNEL.timer -= cycles_elapsed;
        if (NOISE_CHANNEL.timer <= 0)
        {
            NOISE_CHANNEL.timer += get_noise_freq(nes);
            clock_noise_lsfr(nes);
        }
    }

    APU.frame_sequencer.timer -= cycles_elapsed;
    
    while (APU.frame_sequencer.timer <= 0)
    {
        APU.frame_sequencer.timer += APU_FRAME_SEQUENCER_STEP_RATE << 1;
        frame_sequencer_clock(nes);
    }

    // only tick if the user has provided a sample callback
    if (nes->apu_callback && nes->apu_callback_freq)
    {
        nes->apu_callback_counter -= cycles_elapsed;

        if (nes->apu_callback_counter <= 0)
        {
            nes->apu_callback_counter += (CPU_CLOCK / nes->apu_callback_freq);
            sample(nes);
        }
    }
}

void nes_apu_init(struct NES_Core* nes)
{
    // setup noise lsfr
    nes->apu.noise.lsfr = 0x7FF;

    // i assume all channels start enabled...
    nes->apu.status.square1_enable = 1;
    nes->apu.status.square2_enable = 1;
    nes->apu.status.triangle_enable = 1;
    nes->apu.status.noise_enable = 1;
    nes->apu.status.dmc_enable = 1;
}
