#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


#define NES_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define NES_MAX(x, y) (((x) > (y)) ? (x) : (y))

// msvc prepro has a hard time with (macro && macro), so they have to be
// split into different if, else chains...
#if defined(__has_builtin)
#if __has_builtin(__builtin_expect)
#define LIKELY(c) (__builtin_expect(c,1))
#define UNLIKELY(c) (__builtin_expect(c,0))
#else
#define LIKELY(c) ((c))
#define UNLIKELY(c) ((c))
#endif // __has_builtin(__builtin_expect)
#else
#define LIKELY(c) ((c))
#define UNLIKELY(c) ((c))
#endif // __has_builtin

#if defined(__has_builtin)
#if __has_builtin(__builtin_unreachable)
#define NES_UNREACHABLE(ret) __builtin_unreachable()
#else
#define NES_UNREACHABLE(ret) return ret
#endif // __has_builtin(__builtin_unreachable)
#else
#define NES_UNREACHABLE(ret) return ret
#endif // __has_builtin

// used mainly in debugging when i want to quickly silence
// the compiler about unsed vars.
#define NES_UNUSED(var) ((void)(var))

// ONLY use this for C-arrays, not pointers, not structs
#define NES_ARR_SIZE(array) (sizeof(array) / sizeof(array[0]))


#ifdef NES_DEBUG
#include <stdio.h>
#include <assert.h>
#define NES_log(...) fprintf(stdout, __VA_ARGS__)
#define NES_log_err(...) fprintf(stderr, __VA_ARGS__)
#else
#define NES_log(...)
#define NES_log_err(...)
#endif // NES_DEBUG


struct NES_Core; // fwd

bool NES_has_mapper(const uint8_t mapper);
int NES_mapper_setup(struct NES_Core* nes, uint8_t mapper);
void NES_cpu_run(struct NES_Core* nes);
void NES_ppu_run(struct NES_Core* nes);

uint8_t NES_cart_read(struct NES_Core* nes, uint16_t addr);
void NES_cart_write(struct NES_Core* nes, uint16_t addr, uint8_t value);

uint8_t NES_cpu_read(struct NES_Core* nes, uint16_t addr);
void NES_cpu_write(struct NES_Core* nes, uint16_t addr, uint8_t value);
uint16_t NES_cpu_read16(struct NES_Core* nes, uint16_t addr);
void NES_cpu_write16(struct NES_Core* nes, uint16_t addr, uint16_t value);

void NES_dma(struct NES_Core* nes);
uint8_t NES_ppu_read(struct NES_Core* nes, uint16_t addr);
void NES_ppu_write(struct NES_Core* nes, uint16_t addr, uint8_t value);


#ifdef __cplusplus
}
#endif
