#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#if defined __has_builtin
    #define HAS_BUILTIN(x) __has_builtin(x)
#else
    #define HAS_BUILTIN(x) (0)
#endif // __has_builtin

#if HAS_BUILTIN(__builtin_expect)
    #define LIKELY(c) (__builtin_expect(c,1))
    #define UNLIKELY(c) (__builtin_expect(c,0))
#else
    #define LIKELY(c) (c)
    #define UNLIKELY(c) (c)
#endif // __has_builtin(__builtin_expect)

#if HAS_BUILTIN(__builtin_unreachable)
    #define UNREACHABLE(ret) __builtin_unreachable()
#else
    #define UNREACHABLE(ret) return ret
#endif // __has_builtin(__builtin_unreachable)

// used mainly in debugging when i want to quickly silence
// the compiler about unsed vars.
#define UNUSED(var) ((void)(var))

// ONLY use this for C-arrays, not pointers, not structs
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#if NES_DEBUG
    #include <stdio.h>
    #include <assert.h>
    #define NES_log(...) fprintf(stdout, __VA_ARGS__)
    #define NES_log_err(...) fprintf(stderr, __VA_ARGS__)
    #define NES_log_fatal(...) fprintf(stderr, __VA_ARGS__); assert(0)
#else
    #define NES_log(...)
    #define NES_log_err(...)
    #define NES_log_fatal(...)
#endif // NES_DEBUG


enum NesClocks
{
    // 21mhz (in hz)
    NES_MASTER_CLOCK = 21477270,

    // ~1.79mhz (in hz)
    NES_CPU_CYCLES = NES_MASTER_CLOCK / 12,

    NES_CPU_CYCLES_PER_FRAME = NES_CPU_CYCLES / 60,

    NES_APU_FRAME_SEQUENCER_CLOCK = NES_MASTER_CLOCK / 89490,

    NES_APU_FRAME_SEQUENCER_STEP_RATE = NES_CPU_CYCLES / NES_APU_FRAME_SEQUENCER_CLOCK,
};


enum NesInterruptVector
{
    NES_VECTOR_NMI    = 0xFFFA,
    NES_VECTOR_RESET  = 0xFFFC,
    NES_VECTOR_IRQ    = 0xFFFE,
    NES_VECTOR_BRK    = 0xFFFE,
};


struct NES_Core; // fwd


bool nes_has_mapper(const uint8_t mapper);
bool nes_mapper_setup(struct NES_Core* nes, uint8_t mapper);

void nes_cpu_run(struct NES_Core* nes);
void nes_ppu_run(struct NES_Core* nes, const uint16_t cycles_elapsed);
void nes_apu_run(struct NES_Core* nes, const uint16_t cycles_elapsed);

uint8_t nes_cart_read(struct NES_Core* nes, uint16_t addr);
void nes_cart_write(struct NES_Core* nes, uint16_t addr, uint8_t value);

uint8_t nes_cpu_read(struct NES_Core* nes, uint16_t addr);
void nes_cpu_write(struct NES_Core* nes, uint16_t addr, uint8_t value);
uint16_t nes_cpu_read16(struct NES_Core* nes, uint16_t addr);
void nes_cpu_write16(struct NES_Core* nes, uint16_t addr, uint16_t value);

void nes_dma(struct NES_Core* nes);
uint8_t nes_ppu_read(struct NES_Core* nes, uint16_t addr);
void nes_ppu_write(struct NES_Core* nes, uint16_t addr, uint8_t value);

void nes_cpu_nmi(struct NES_Core* nes);

uint8_t nes_joypad_read_port_0(struct NES_Core* nes);

uint8_t nes_apu_io_read(struct NES_Core* nes, const uint16_t addr);
void nes_apu_io_write(struct NES_Core* nes, const uint16_t addr, const uint8_t value);


void ctrl_set_nametable(struct NES_Core* nes, uint8_t v);
void ctrl_set_vram_addr(struct NES_Core* nes, uint8_t v);
void ctrl_set_obj_8x8_addr(struct NES_Core* nes, uint8_t v);
void ctrl_set_bg_addr(struct NES_Core* nes, uint8_t v);
void ctrl_set_obj_size(struct NES_Core* nes, uint8_t v);
void ctrl_set_master(struct NES_Core* nes, uint8_t v);
void ctrl_set_nmi(struct NES_Core* nes, uint8_t v);
void mask_set_greyscale(struct NES_Core* nes, uint8_t v);
void mask_set_bg_leftmost(struct NES_Core* nes, uint8_t v);
void mask_set_obj_leftmost(struct NES_Core* nes, uint8_t v);
void mask_set_bg_on(struct NES_Core* nes, uint8_t v);
void mask_set_obj_on(struct NES_Core* nes, uint8_t v);
void mask_set_bgr(struct NES_Core* nes, uint8_t v);
void status_set_lsb(struct NES_Core* nes, uint8_t v);
void status_set_obj_overflow(struct NES_Core* nes, uint8_t v);
void status_set_obj_hit(struct NES_Core* nes, uint8_t v);
void status_set_vblank(struct NES_Core* nes, uint8_t v);

uint8_t ctrl_get_nametable(const struct NES_Core* nes);
uint8_t ctrl_get_vram_addr(const struct NES_Core* nes);
uint8_t ctrl_get_obj_8x8_addr(const struct NES_Core* nes);
uint8_t ctrl_get_bg_addr(const struct NES_Core* nes);
uint8_t ctrl_get_obj_size(const struct NES_Core* nes);
uint8_t ctrl_get_master(const struct NES_Core* nes);
uint8_t ctrl_get_nmi(const struct NES_Core* nes);
uint8_t mask_get_greyscale(const struct NES_Core* nes);
uint8_t mask_get_bg_leftmost(const struct NES_Core* nes);
uint8_t mask_get_obj_leftmost(const struct NES_Core* nes);
uint8_t mask_get_bg_on(const struct NES_Core* nes);
uint8_t mask_get_obj_on(const struct NES_Core* nes);
uint8_t mask_get_bgr(const struct NES_Core* nes);
uint8_t status_get_lsb(const struct NES_Core* nes);
uint8_t status_get_obj_overflow(const struct NES_Core* nes);
uint8_t status_get_obj_hit(const struct NES_Core* nes);
uint8_t status_get_vblank(const struct NES_Core* nes);

#ifdef __cplusplus
}
#endif
