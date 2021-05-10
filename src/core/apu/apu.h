#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../types.h"


#define APU nes->apu

#define SQUARE1_CHANNEL APU.square1
#define SQUARE2_CHANNEL APU.square2
#define TRIANGLE_CHANNEL APU.triangle
#define NOISE_CHANNEL APU.noise
#define DMC_CHANNEL APU.dmc

#define STATUS APU.status
#define FRAME_SEQUENCER APU.frame_sequencer


#ifndef NES_VOLUME_SCALE
#define NES_VOLUME_SCALE 8 /* max range of -120 and +120 */
#endif // NES_VOLUME_SCALE


// defined in core/apu/apu.c
extern const bool SQUARE_DUTY[4][8];
extern const uint8_t LENGTH_COUNTER_TABLE[0x20];
extern const uint16_t NOISE_TIMER_TABLE[0x10];
extern const int8_t TRIANGLE_DUTY_TABLE[0x20];


// [SQUARE1 - PULSE1]
bool is_square1_enabled(const struct NES_Core* nes);
bool is_square1_length_enabled(const struct NES_Core* nes);
uint16_t get_square1_freq(const struct NES_Core* nes);
void clock_square1_length(struct NES_Core* nes);
void clock_square1_envelope(struct NES_Core* nes);
void clock_square1_sweep(struct NES_Core* nes);
void clock_square1_duty(struct NES_Core* nes);
int8_t sample_square1(const struct NES_Core* nes);


// [SQUARE2 - PULSE2]
bool is_square2_enabled(const struct NES_Core* nes);
bool is_square2_length_enabled(const struct NES_Core* nes);
uint16_t get_square2_freq(const struct NES_Core* nes);
void clock_square2_length(struct NES_Core* nes);
void clock_square2_envelope(struct NES_Core* nes);
void clock_square2_sweep(struct NES_Core* nes);
void clock_square2_duty(struct NES_Core* nes);
int8_t sample_square2(const struct NES_Core* nes);


// [TRIANGLE]
bool is_triangle_enabled(const struct NES_Core* nes);
bool is_triangle_length_enabled(const struct NES_Core* nes);
uint16_t get_triangle_freq(const struct NES_Core* nes);
void clock_triangle_length(struct NES_Core* nes);
void clock_triangle_linear(struct NES_Core* nes);
void clock_triangle_duty(struct NES_Core* nes);
int8_t sample_triangle(const struct NES_Core* nes);


// [NOISE]
bool is_noise_enabled(const struct NES_Core* nes);
bool is_noise_length_enabled(const struct NES_Core* nes);
uint16_t get_noise_freq(const struct NES_Core* nes);
void clock_noise_length(struct NES_Core* nes);
void clock_noise_envelope(struct NES_Core* nes);
void clock_noise_lsfr(struct NES_Core* nes);
int8_t sample_noise(const struct NES_Core* nes);


#ifdef __cplusplus
}
#endif
