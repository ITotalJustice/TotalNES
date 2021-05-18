#include "../nes.h"
#include "../internal.h"
#include "apu.h"

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

// this is the volume for the triangle
// the table isn't accurate yet, just oprox, need to correct it more
// for signed values
const int8_t TRIANGLE_DUTY_TABLE[0x20] = {
  -7, -6, -5, -4, -3, -2, -1, -0,
  +1, +2, +3, +4, +5, +6, +7, +8,
  +8, +7, +6, +5, +4, +3, +2, +1,
  +0, -1, -2, -3, -4, -5, -6, -7
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
  clock_triangle_length(nes);
  clock_noise_length(nes);

  clock_square1_sweep(nes);
  clock_square2_sweep(nes);
}

static void on_clock_envelope_and_linear_counter(struct NES_Core* nes) {
  clock_square1_envelope(nes);
  clock_square2_envelope(nes);
  clock_noise_envelope(nes);

  clock_triangle_linear(nes);
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

static inline int8_t builtin_mixer(const struct NES_Core* nes, const struct GB_MixerData data) {
  (void)nes;
  // this mode is just for testing each channel
  // it will be removed at somepoint!
  #define MODE 0

  #if MODE == 3
      return data.triangle * NES_VOLUME_SCALE;

  #elif MODE == 4
      return data.noise * NES_VOLUME_SCALE;
  #else
      return ((data.square1 * NES_VOLUME_SCALE) + (data.square2 * NES_VOLUME_SCALE) +
              (data.triangle * NES_VOLUME_SCALE) + (data.noise * NES_VOLUME_SCALE)) / 4;
  #endif
}


static void sample(struct NES_Core* nes) {
  const struct GB_MixerData mixer_data = {
    .square1 = sample_square1(nes) * is_square1_enabled(nes),
    .square2 = sample_square2(nes) * is_square2_enabled(nes),
    .triangle = sample_triangle(nes) * is_triangle_enabled(nes),
    .noise = sample_noise(nes) * is_noise_enabled(nes),
  };

  // check if the user has set it's own mixer callback!
  if (nes->mixer_cb != NULL) {
    APU.sample_data.samples[APU.sample_index] = nes->mixer_cb(
      nes, nes->user_data, mixer_data
    );
  }

  // use our own!
  else {
    APU.sample_data.samples[APU.sample_index] = builtin_mixer(
      nes, mixer_data
    );
  }

  // mono
  ++APU.sample_index;

  if (APU.sample_index >= NES_ARR_SIZE(APU.sample_data.samples)) {
    APU.sample_index = 0;

    if (nes->apu_cb != NULL) {
      nes->apu_cb(nes, nes->user_data, &APU.sample_data);
    }
  }
}

void nes_apu_run(struct NES_Core* nes, const uint16_t cycles_elapsed) {
  // atm im multiplying the freq by 2, it sounds too high pitch at normal
  // frequency for some reason...
  // i read that its 2 cpu cycles for 1 apu cycle, so maybe thats why?

  SQUARE1_CHANNEL.timer -= cycles_elapsed;
  if (SQUARE1_CHANNEL.timer <= 0) {
    SQUARE1_CHANNEL.timer += get_square1_freq(nes) << 1;
    clock_square1_duty(nes);
  }

  SQUARE2_CHANNEL.timer -= cycles_elapsed;
  if (SQUARE2_CHANNEL.timer <= 0) {
    SQUARE2_CHANNEL.timer += get_square2_freq(nes) << 1;
    clock_square2_duty(nes);
  }

  TRIANGLE_CHANNEL.timer -= cycles_elapsed;
  if (TRIANGLE_CHANNEL.timer <= 0) {
    TRIANGLE_CHANNEL.timer += get_triangle_freq(nes);
    clock_triangle_duty(nes);
  }

  NOISE_CHANNEL.timer -= cycles_elapsed;
  if (NOISE_CHANNEL.timer <= 0) {
    NOISE_CHANNEL.timer += get_noise_freq(nes);
    clock_noise_lsfr(nes);
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
