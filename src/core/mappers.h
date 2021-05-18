#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>


/* https://wiki.nesdev.com/w/index.php/NROM */
struct NES_Mapper_000 {
  uint8_t* prg_rom_slots[2];
  uint8_t* prg_ram_slots[2];
  uint8_t* chr_ram_slots[4];
  uint8_t prg_ram[0x1000];    /* 4k */
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/MMC1 */
struct NES_Mapper_001 {
  uint8_t prg_ram[0x8000];    /* 32k */
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/UxROM */
struct NES_Mapper_002 {
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/INES_Mapper_003 */
struct NES_Mapper_003 {
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/MMC3 */
struct NES_Mapper_004 {
  uint8_t prg_ram[0x2000];    /* 8k */
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/MMC5 */
struct NES_Mapper_005 {
  uint8_t prg_ram[0x20000];   /* 128k */
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/MMC2 */
struct NES_Mapper_009 {
  uint8_t prg_ram[0x2000];    /* 8k */
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/MMC4 */
struct NES_Mapper_010 {
  uint8_t prg_ram[0x2000];    /* 8k */
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/INES_Mapper_037 */
struct NES_Mapper_037 {
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/INES_Mapper_047 */
struct NES_Mapper_047 {
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/GxROM */
struct NES_Mapper_066 {
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/INES_Mapper_099 */
struct NES_Mapper_099 {
  uint8_t prg_ram[0x800];     /* 2k */
  uint8_t chr_ram[0x2000];    /* 8k */
};

/* https://wiki.nesdev.com/w/index.php/INES_Mapper_105 */
struct NES_Mapper_105 {
  uint8_t prg_ram[0x8000];    /* 32k */
  uint8_t chr_ram[0x2000];    /* 8k */
};

#ifdef __cplusplus
}
#endif
