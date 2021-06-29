#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

// in debug builds, we don't want to force inline anything
#if NES_DEBUG
    #define FORCE_INLINE
#else
    #if defined(_MSC_VER)
        #define FORCE_INLINE inline __forceinline
    #elif defined(__GNUC__)
        #define FORCE_INLINE inline __attribute__((always_inline))
    #elif defined(__clang__)
        #define FORCE_INLINE inline __attribute__((always_inline))
    #else
        #define FORCE_INLINE inline
    #endif
#endif

#if NES_SINGLE_FILE
    #define NES_STATIC static
    #define NES_INLINE static inline
    #define NES_FORCE_INLINE static FORCE_INLINE
#else
    #define NES_STATIC
    #define NES_INLINE
    #define NES_FORCE_INLINE
#endif // NES_SINGLE_FILE

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
    #define NES_log_fatal(...) do { fprintf(stderr, __VA_ARGS__); assert(0); } while(0)
#else
    #define NES_log(...)
    #define NES_log_err(...)
    #define NES_log_fatal(...)
#endif // NES_DEBUG

// returns 1 OR 0
#define IS_BIT_SET(v, bit) (!!((v) & (1 << (bit))))

enum
{
    // 21mhz (in hz)
    MASTER_CLOCK = 21477270,

    // ~1.79mhz (in hz)
    CPU_CLOCK = MASTER_CLOCK / 12,

    CPU_CYCLES_PER_FRAME = CPU_CLOCK / 60,

    APU_FRAME_SEQUENCER_CLOCK = MASTER_CLOCK / 89490,

    APU_FRAME_SEQUENCER_STEP_RATE = CPU_CLOCK / APU_FRAME_SEQUENCER_CLOCK,
};


enum InterruptVector
{
    VECTOR_NMI    = 0xFFFA,
    VECTOR_RESET  = 0xFFFC,
    VECTOR_IRQ    = 0xFFFE,
    VECTOR_BRK    = 0xFFFE,
};

enum Mirror
{
    ONE_SCREEN_LOW,
    ONE_SCREEN_HIGH,
    HORIZONTAL,
    VERTICAL,
    FOUR_SCREEN,
};

struct NES_Core; // fwd


NES_STATIC void nes_apu_init(struct NES_Core* nes);
NES_STATIC void nes_ppu_init(struct NES_Core* nes);

NES_STATIC bool nes_mapper_get_prg_chr_ram_size(uint8_t mapper, size_t* prg_size, size_t* chr_size);
NES_STATIC bool nes_mapper_setup(struct NES_Core* nes, uint8_t mapper, enum Mirror mirror);

NES_FORCE_INLINE void nes_cpu_run(struct NES_Core* nes);
NES_FORCE_INLINE void nes_ppu_run(struct NES_Core* nes, const uint16_t cycles_elapsed);
NES_FORCE_INLINE void nes_apu_run(struct NES_Core* nes, const uint16_t cycles_elapsed);

NES_INLINE uint8_t nes_cart_read(struct NES_Core* nes, uint16_t addr);
NES_INLINE void nes_cart_write(struct NES_Core* nes, uint16_t addr, uint8_t value);

NES_FORCE_INLINE uint8_t nes_cpu_read(struct NES_Core* nes, uint16_t addr);
NES_FORCE_INLINE void nes_cpu_write(struct NES_Core* nes, uint16_t addr, uint8_t value);
NES_FORCE_INLINE uint16_t nes_cpu_read16(struct NES_Core* nes, uint16_t addr);
// NES_FORCE_INLINE void nes_cpu_write16(struct NES_Core* nes, uint16_t addr, uint16_t value);

NES_STATIC void nes_dma(struct NES_Core* nes);
NES_FORCE_INLINE uint8_t nes_ppu_read(struct NES_Core* nes, uint16_t addr);
NES_FORCE_INLINE void nes_ppu_write(struct NES_Core* nes, uint16_t addr, uint8_t value);

NES_STATIC void nes_cpu_nmi(struct NES_Core* nes);

NES_INLINE uint8_t nes_joypad_read_port_0(struct NES_Core* nes);
NES_STATIC void nes_joypad_write(struct NES_Core* nes, uint8_t value);

NES_STATIC uint8_t nes_apu_io_read(struct NES_Core* nes, const uint16_t addr);
NES_INLINE void nes_apu_io_write(struct NES_Core* nes, const uint16_t addr, const uint8_t value);

NES_FORCE_INLINE void status_set_obj_overflow(struct NES_Core* nes, uint8_t v);
NES_FORCE_INLINE void status_set_obj_hit(struct NES_Core* nes, uint8_t v);
NES_FORCE_INLINE void status_set_vblank(struct NES_Core* nes, uint8_t v);

NES_STATIC uint8_t ctrl_get_vram_addr(const struct NES_Core* nes);
NES_STATIC uint8_t ctrl_get_nmi(const struct NES_Core* nes);

#ifdef __cplusplus
}
#endif
