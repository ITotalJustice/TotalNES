#include "../nes.h"
#include "../internal.h"
#include "apu.h"

#include <assert.h>


uint8_t nes_apu_io_read(struct NES_Core* nes, const uint16_t addr) {
  if (addr == 0x4015) {
    uint8_t data = nes->apu.io[addr & 0x1F] & ~(0xF);

    data |= (SQUARE1_CHANNEL.length_counter > 0) << 0;
    data |= (SQUARE2_CHANNEL.length_counter > 0) << 1;
    data |= ((TRIANGLE_CHANNEL.length_counter > 0) && (TRIANGLE_CHANNEL.linear_counter_load > 0)) << 2;
    data |= (NOISE_CHANNEL.length_counter > 0) << 3;
    
    nes->apu.status.frame_irq = 0;

    return data;
}
  else {
    // reading from these registers returns the last written value,
    // NOT the current value stored!
    return APU.io[addr & 0x1F];
  }
}


void nes_apu_io_write(struct NES_Core* nes, const uint16_t addr, const uint8_t value) {
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
      SQUARE1_CHANNEL.sweep_negate = (value >> 3) & 0x1;
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
      SQUARE2_CHANNEL.sweep_negate = (value >> 3) & 0x1;
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
      TRIANGLE_CHANNEL.linear_counter_load = value & 0x7F;
      TRIANGLE_CHANNEL.length_counter_halt = (value >> 7) & 0x1;
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
