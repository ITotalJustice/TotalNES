#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/types.h"


struct NES_State;

// API
void NES_reset(struct NES_Core* nes);
int NES_is_header_valid(struct NES_Core* nes, const struct NES_CartHeader* header);
int NES_loadrom(struct NES_Core* nes, uint8_t* rom, size_t size);
void NES_run_frame(struct NES_Core* nes);

// NOT DONE
int NES_savestate(struct NES_Core* nes, struct NES_State* state);
int NES_loadstate(struct NES_Core* nes, const struct NES_State* state);


struct NES_PatternTableGfx {
    uint32_t pixels[128][128];
};

struct NES_PatternTableGfx NES_ppu_get_pattern_table(struct NES_Core* nes, uint8_t table_index, uint8_t palette);

#ifdef __cplusplus
}
#endif
