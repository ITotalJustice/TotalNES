#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../types.h"


enum
{
    PRG_ROM_BANK_SIZE = 1024 * 16,
};


NES_FORCE_INLINE bool    mapper_init_000(struct NES_Core* nes, enum Mirror mirror);
NES_FORCE_INLINE uint8_t mapper_read_000(struct NES_Core* nes, uint16_t addr);
NES_FORCE_INLINE void    mapper_write_000(struct NES_Core* nes, uint16_t addr, uint8_t value);

NES_FORCE_INLINE bool    mapper_init_001(struct NES_Core* nes, enum Mirror mirror);
NES_FORCE_INLINE uint8_t mapper_read_001(struct NES_Core* nes, uint16_t addr);
NES_FORCE_INLINE void    mapper_write_001(struct NES_Core* nes, uint16_t addr, uint8_t value);

NES_FORCE_INLINE bool    mapper_init_002(struct NES_Core* nes, enum Mirror mirror);
NES_FORCE_INLINE uint8_t mapper_read_002(struct NES_Core* nes, uint16_t addr);
NES_FORCE_INLINE void    mapper_write_002(struct NES_Core* nes, uint16_t addr, uint8_t value);

NES_FORCE_INLINE bool    mapper_init_003(struct NES_Core* nes, enum Mirror mirror);
NES_FORCE_INLINE uint8_t mapper_read_003(struct NES_Core* nes, uint16_t addr);
NES_FORCE_INLINE void    mapper_write_003(struct NES_Core* nes, uint16_t addr, uint8_t value);


#ifdef __cplusplus
}
#endif
