#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../types.h"


enum
{
    PRG_ROM_BANK_SIZE = 1024 * 16,
};


// internal low level funcs to set ptr's to table.
// most mappers will not use these
NES_STATIC void mapper_set_pattern_table(struct NES_Core* nes, uint8_t table, const uint8_t* rptr, uint8_t* wptr);
NES_STATIC void mapper_set_nametable(struct NES_Core* nes, uint8_t table, const uint8_t* rptr, uint8_t* wptr);

NES_STATIC void mapper_set_pattern_table_bank(struct NES_Core* nes, uint8_t table, uint32_t offset);
NES_STATIC void mapper_set_nametable_mirroring(struct NES_Core* nes, enum Mirror mirror);




NES_STATIC bool mapper_init_000(struct NES_Core* nes, enum Mirror mirror);
NES_FORCE_INLINE uint8_t mapper_read_000(struct NES_Core* nes, uint16_t addr);
NES_FORCE_INLINE void mapper_write_000(struct NES_Core* nes, uint16_t addr, uint8_t value);
NES_STATIC void mapper_get_prg_chr_ram_size_000(size_t* prg_size, size_t* chr_size);

NES_STATIC bool mapper_init_001(struct NES_Core* nes, enum Mirror mirror);
NES_FORCE_INLINE uint8_t mapper_read_001(struct NES_Core* nes, uint16_t addr);
NES_FORCE_INLINE void mapper_write_001(struct NES_Core* nes, uint16_t addr, uint8_t value);
NES_STATIC void mapper_get_prg_chr_ram_size_001(size_t* prg_size, size_t* chr_size);

NES_STATIC bool mapper_init_002(struct NES_Core* nes, enum Mirror mirror);
NES_FORCE_INLINE uint8_t mapper_read_002(struct NES_Core* nes, uint16_t addr);
NES_FORCE_INLINE void mapper_write_002(struct NES_Core* nes, uint16_t addr, uint8_t value);
NES_STATIC void mapper_get_prg_chr_ram_size_002(size_t* prg_size, size_t* chr_size);

NES_STATIC bool mapper_init_003(struct NES_Core* nes, enum Mirror mirror);
NES_FORCE_INLINE uint8_t mapper_read_003(struct NES_Core* nes, uint16_t addr);
NES_FORCE_INLINE void mapper_write_003(struct NES_Core* nes, uint16_t addr, uint8_t value);
NES_STATIC void mapper_get_prg_chr_ram_size_003(size_t* prg_size, size_t* chr_size);

NES_STATIC bool mapper_init_007(struct NES_Core* nes, enum Mirror mirror);
NES_FORCE_INLINE uint8_t mapper_read_007(struct NES_Core* nes, uint16_t addr);
NES_FORCE_INLINE void mapper_write_007(struct NES_Core* nes, uint16_t addr, uint8_t value);
NES_STATIC void mapper_get_prg_chr_ram_size_007(size_t* prg_size, size_t* chr_size);

#ifdef __cplusplus
}
#endif
