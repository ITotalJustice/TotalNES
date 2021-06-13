#include "nes.h"
#include "internal.h"
#include "tables/cycle_table.h"


/* cpu registers */
#define REG_PC  nes->cpu.PC
#define REG_A   nes->cpu.A
#define REG_X   nes->cpu.X
#define REG_Y   nes->cpu.Y
#define REG_SP  nes->cpu.S

/* status flags */
#define CARRY     nes->cpu.C
#define ZERO      nes->cpu.Z
#define INTERRUPT nes->cpu.I
#define DECIMAL   nes->cpu.D
#define BFLAG     nes->cpu.B
#define OVERFLOW  nes->cpu.V
#define NEGATIVE  nes->cpu.N

#define GET_REG_P() (   \
    (NEGATIVE   << 7)   | \
    (OVERFLOW   << 6)   | \
    (0x01       << 5)   | \
    (0x01       << 4)   | \
    (DECIMAL    << 3)   | \
    (INTERRUPT  << 2)   | \
    (ZERO       << 1)   | \
    (CARRY      << 0)     \
)

#define SET_REG_P(v) \
    NEGATIVE  = (v & 0x80) > 0; \
    OVERFLOW  = (v & 0x40) > 0; \
    DECIMAL   = (v & 0x08) > 0; \
    INTERRUPT = (v & 0x04) > 0; \
    ZERO      = (v & 0x02) > 0; \
    CARRY     = (v & 0x01) > 0;

#define read8(addr)         nes_cpu_read(nes, addr)
#define read16(addr)        nes_cpu_read16(nes, addr)
#define write8(addr,value)  nes_cpu_write(nes, addr, value)
#define write16(addr,value) nes_cpu_write16(nes, addr, value)

/* branchless pagecross. */
#if 1
    #define PAGECROSS(a,b) nes->cpu.cycles += (CYCLE_PAIR_TABLE[opcode].p >> (((a) & 0x0F00) == (((a) + (b)) & 0x0F00)))
#else
    #define PAGECROSS(a,b) nes->cpu.cycles += ((a) & 0x0F00) == (((a) +(b)) & 0x0F00) ? 0 : CYCLE_PAIR_TABLE[opcode].p
#endif

/*START: ADDRESSING MODES*/
#define IMP() /*non*/
#define ACC() /*non*/
#define REL() do { oprand = REG_PC++; } while(0)
#define IMM() do { REL();             } while(0)

#define ABS() do { \
    oprand = read16(REG_PC); \
    REG_PC += 2; \
} while(0)

#define _ABSXY(reg) do { \
    ABS(); \
    PAGECROSS(oprand, reg); \
    oprand += reg; \
} while(0)

#define ABSX() do { _ABSXY(REG_X); } while(0)
#define ABSY() do { _ABSXY(REG_Y); } while(0)

#define ZP()  do { oprand = read8(REG_PC++);                  } while(0)
#define ZPX() do { oprand = (read8(REG_PC++) + REG_X) & 0xFF; } while(0)
#define ZPY() do { oprand = (read8(REG_PC++) + REG_Y) & 0xFF; } while(0)

#define IND() do { \
    const uint16_t oprand_tmp = read16(REG_PC); \
    REG_PC += 2; \
    oprand = (uint16_t)read8((oprand_tmp & 0xFF00) | ((oprand_tmp + 1) & 0xFF)) << 8; \
    oprand |= read8(oprand_tmp); \
} while(0)

#define INDX() do { \
    const uint8_t oprand_tmp = read8(REG_PC++); \
    oprand = (uint16_t)read8(oprand_tmp + REG_X + 1) << 8; \
    oprand |= read8(oprand_tmp + REG_X); \
} while(0)

#define INDY() do { \
    const uint8_t oprand_tmp = read8(REG_PC++); \
    oprand = (uint16_t)read8(oprand_tmp + 1) << 8; \
    oprand |= read8(oprand_tmp); \
    PAGECROSS(oprand, REG_Y); \
    oprand += REG_Y; \
} while(0)
/*END: ADDRESSING MODES*/

#define SET_FLAGS_ZN(z,n) do { \
    ZERO = (z) == 0; \
    NEGATIVE = ((n) & 0x80) == 0x80; \
} while(0)

/*START: STACK HELPERS*/
static inline uint8_t _pop8(struct NES_Core* nes)
{
    return read8(++REG_SP | 0x100);
}
static inline uint16_t _pop16(struct NES_Core* nes)
{
    const uint8_t lo = _pop8(nes); return lo | (_pop8(nes) << 8);
}
static inline void _push8(struct NES_Core* nes, const uint8_t v)
{
    write8(REG_SP-- | 0x100, v);
}
static inline void _push16(struct NES_Core* nes, const uint16_t v)
{
    _push8(nes, (v >> 8) & 0xFF); _push8(nes, v & 0xFF);
}

#define POP8()        _pop8(nes)
#define POP16()       _pop16(nes)
#define PUSH8(value)  _push8(nes, value)
#define PUSH16(value) _push16(nes, value)
/*END: STACK HELPERS*/

/*START: JUMPS*/
#define JSR() do { \
    PUSH16(REG_PC - 1); \
    REG_PC = oprand; \
} while(0)

#define RTI() do { \
    const uint8_t r = (POP8() & 0xEF); \
    SET_REG_P(r); \
    REG_PC = POP16(); \
} while(0)

#define JMP() do { REG_PC = oprand;       } while(0)
#define RTS() do { REG_PC = POP16() + 1;  } while(0)
/*END: JUMPS*/

/*START: LOADS*/
#define LOAD(reg) do { \
    reg = read8(oprand); \
    SET_FLAGS_ZN(reg, reg); \
} while(0)

#define LDA() do { LOAD(REG_A); } while(0)
#define LDX() do { LOAD(REG_X); } while(0)
#define LDY() do { LOAD(REG_Y); } while(0)
/*END: LOADS*/

/*START: STORE*/
#define STORE(reg)  do { write8(oprand, reg); } while(0)
#define STA()       do { STORE(REG_A);        } while(0)
#define STX()       do { STORE(REG_X);        } while(0)
#define STY()       do { STORE(REG_Y);        } while(0)
/*END: STORE*/

#define NOP() /*no operation*/
#define DOP() /*double nop*/
#define TOP() /*tripple nop*/
#define STP() /*stops the cpu*/

/*START: FLAG OPS*/
#define CLC() do { CARRY = 0;     } while(0)
#define CLI() do { INTERRUPT = 0; } while(0)
#define CLV() do { OVERFLOW = 0;  } while(0)
#define CLD() do { DECIMAL = 0;   } while(0)
#define SEC() do { CARRY = 1;     } while(0)
#define SEI() do { INTERRUPT = 1; } while(0)
#define SED() do { DECIMAL = 1;   } while(0)
/*END: FLAG OPS*/

/*START: BRANCH*/
#define BRANCH(cond) do { \
    oprand = read8(oprand); \
    if (cond) { \
        PAGECROSS(REG_PC, (int8_t)oprand); \
        REG_PC += (int8_t)oprand; \
        nes->cpu.cycles += 1; \
    } \
} while(0)

#define BCC() do { BRANCH(!CARRY);    } while(0)
#define BCS() do { BRANCH(CARRY);     } while(0)
#define BNE() do { BRANCH(!ZERO);     } while(0)
#define BEQ() do { BRANCH(ZERO);      } while(0)
#define BPL() do { BRANCH(!NEGATIVE); } while(0)
#define BMI() do { BRANCH(NEGATIVE);  } while(0)
#define BVC() do { BRANCH(!OVERFLOW); } while(0)
#define BVS() do { BRANCH(OVERFLOW);  } while(0)
/*END: BRANCH*/

#define BIT() do { \
    oprand = read8(oprand); \
    OVERFLOW = ((oprand & 0x40) == 0x40); \
    SET_FLAGS_ZN((oprand & REG_A), oprand); \
} while(0)

/*START: STACK OPS*/
#define PLA() do { \
  REG_A = POP8(); \
  SET_FLAGS_ZN(REG_A, REG_A); \
} while(0)

#define PLP() do { const uint8_t r = (POP8() & 0xEF); SET_REG_P(r); } while(0)
#define PHA() do { PUSH8(REG_A); } while(0)
#define PHP() do { PUSH8(GET_REG_P() | 0x10); } while(0)
/*END: STACK OPS*/

#define AND() do { \
    REG_A &= read8(oprand); \
    SET_FLAGS_ZN(REG_A, REG_A); \
} while(0)

#define COMP(reg) do { \
    oprand = read8(oprand); \
    CARRY = reg >= oprand; \
    oprand = (reg - oprand) & 0xFF; \
    SET_FLAGS_ZN(oprand, oprand); \
} while(0)

#define CMP() do { COMP(REG_A); } while(0)
#define CPX() do { COMP(REG_X); } while(0)
#define CPY() do { COMP(REG_Y); } while(0)

#define ORA() do { \
    REG_A |= read8(oprand); \
    SET_FLAGS_ZN(REG_A, REG_A); \
} while(0)

#define EOR() do { \
    REG_A ^= read8(oprand); \
    SET_FLAGS_ZN(REG_A, REG_A); \
} while(0)

/*START: ADC / SBC*/
#define ADCSBC() do { \
    const uint8_t old_carry = CARRY; \
    const uint8_t old_a = REG_A; \
    CARRY = (REG_A + oprand + CARRY) > 0xFF; \
    REG_A += oprand + old_carry; \
    OVERFLOW = ((old_a ^ REG_A) & (oprand ^ REG_A) & 0x80) > 0; \
    SET_FLAGS_ZN(REG_A, REG_A); \
} while(0)

#define ADC() do { \
    oprand = read8(oprand); \
    ADCSBC(); \
} while(0)

#define SBC() do { \
    oprand = (read8(oprand) ^ 0xFF); \
    ADCSBC(); \
} while(0)
/*END: ADC / SBC*/

#define _DEC(reg) do { \
    --reg; \
    SET_FLAGS_ZN(reg, reg); \
} while(0)

#define _INC(reg) do { \
    ++reg; \
    SET_FLAGS_ZN(reg, reg); \
} while(0)

#define DEX() do { _DEC(REG_X); } while(0)
#define DEY() do { _DEC(REG_Y); } while(0)
#define INX() do { _INC(REG_X); } while(0)
#define INY() do { _INC(REG_Y); } while(0)

#define TRANSFER(a,b) do { \
    b = a; \
    SET_FLAGS_ZN(b, b); \
} while(0)

#define TAX() do { TRANSFER(REG_A, REG_X);  } while(0)
#define TXA() do { TRANSFER(REG_X, REG_A);  } while(0)
#define TAY() do { TRANSFER(REG_A, REG_Y);  } while(0)
#define TYA() do { TRANSFER(REG_Y, REG_A);  } while(0)
#define TSX() do { TRANSFER(REG_SP, REG_X); } while(0)
#define TXS() do { REG_SP = REG_X;          } while(0)

#define _ROR(value) do { \
    const uint8_t old_carry = CARRY; \
    CARRY = value & 1; \
    value = (value >> 1) | (old_carry << 7); \
    SET_FLAGS_ZN(value, value); \
} while(0)

#define ROR() do { \
    uint8_t value = read8(oprand); \
    _ROR(value); \
    write8(oprand, value); \
} while(0)

#define _LSR(value) do { \
    CARRY = value & 1; \
    value >>= 1; \
    SET_FLAGS_ZN(value, 0); \
} while(0)

#define LSR() do { \
    uint8_t value = read8(oprand); \
    _LSR(value); \
    write8(oprand, value); \
} while(0)

#define _ROL(value) do { \
    const uint8_t old_carry = CARRY; \
    CARRY = (value & 0x80) == 0x80; \
    value = (value << 1) | old_carry; \
    SET_FLAGS_ZN(value, value); \
} while(0)

#define ROL() do { \
    uint8_t value = read8(oprand); \
    _ROL(value); \
    write8(oprand, value); \
} while(0)

#define _ASL(value) do { \
    CARRY = (value & 0x80) == 0x80; \
    value <<= 1; \
    SET_FLAGS_ZN(value, value); \
} while(0)

#define ASL() do { \
    uint8_t value = read8(oprand); \
    _ASL(value); \
    write8(oprand, value); \
} while(0)

#define RORA() do { _ROR(REG_A); } while(0)
#define LSRA() do { _LSR(REG_A); } while(0)
#define ROLA() do { _ROL(REG_A); } while(0)
#define ASLA() do { _ASL(REG_A); } while(0)

#define INC() do { \
    const uint8_t result = read8(oprand) + 1; \
    write8(oprand, result); \
    SET_FLAGS_ZN(result, result); \
} while(0)

#define DEC() do { \
    const uint8_t result = read8(oprand) - 1; \
    write8(oprand, result); \
    SET_FLAGS_ZN(result, result); \
} while(0)

#define LAX() do { \
    LDA(); \
    REG_A = REG_X = read8(oprand); \
    SET_FLAGS_ZN(REG_A, REG_A); \
} while(0)

#define SAX() do { write8(oprand, REG_X & REG_A); } while(0)
#define DCP() do { DEC(); COMP(REG_A);            } while(0)
#define ISC() do { INC(); SBC();                  } while(0)
#define SLO() do { ASL(); ORA();                  } while(0)
#define RLA() do { ROL(); AND();                  } while(0)
#define SRE() do { LSR(); EOR();                  } while(0)
#define RRA() do { ROR(); ADC();                  } while(0)


void nes_cpu_nmi(struct NES_Core* nes)
{
    // save the pc
    PUSH16(REG_PC);
    // save the current status
    PUSH8(GET_REG_P());
    // read from nmi vector
    REG_PC = read16(NES_VECTOR_NMI);
}

void nes_cpu_run(struct NES_Core* nes)
{
    nes->cpu.cycles = 0;

    const uint8_t opcode = read8(REG_PC++);
    uint16_t oprand;

    switch (opcode)
    {
        case 0x01: INDX();  ORA();  break;
        case 0x02: IMP();   STP();  break;
        case 0x03: INDX();  SLO();  break;
        case 0x04: ZP();    DOP();  break;
        case 0x05: ZP();    ORA();  break;
        case 0x06: ZP();    ASL();  break;
        case 0x07: ZP();    SLO();  break;
        case 0x08: IMP();   PHP();  break;
        case 0x09: IMM();   ORA();  break;
        case 0x0A: ACC();   ASLA(); break;
        case 0x0C: ABS();   TOP();  break;
        case 0x0D: ABS();   ORA();  break;
        case 0x0E: ABS();   ASL();  break;
        case 0x0F: ABS();   SLO();  break;
        case 0x10: REL();   BPL();  break;
        case 0x11: INDY();  ORA();  break;
        case 0x12: IMP();   STP();  break;
        case 0x13: INDY();  SLO();  break;
        case 0x14: ZPX();   DOP();  break;
        case 0x15: ZPX();   ORA();  break;
        case 0x16: ZPX();   ASL();  break;
        case 0x17: ZPX();   SLO();  break;
        case 0x18: IMP();   CLC();  break;
        case 0x19: ABSY();  ORA();  break;
        case 0x1A: IMP();   NOP();  break;
        case 0x1B: ABSY();  SLO();  break;
        case 0x1C: ABSX();  TOP();  break;
        case 0x1D: ABSX();  ORA();  break;
        case 0x1E: ABSX();  ASL();  break;
        case 0x1F: ABSX();  SLO();  break;
        case 0x20: ABS();   JSR();  break;
        case 0x21: INDX();  AND();  break;
        case 0x22: IMP();   STP();  break;
        case 0x23: INDX();  RLA();  break;
        case 0x24: ZP();    BIT();  break;
        case 0x25: ZP();    AND();  break;
        case 0x26: ZP();    ROL();  break;
        case 0x27: ZP();    RLA();  break;
        case 0x28: IMP();   PLP();  break;
        case 0x29: IMM();   AND();  break;
        case 0x2A: ACC();   ROLA(); break;
        case 0x2C: ABS();   BIT();  break;
        case 0x2D: ABS();   AND();  break;
        case 0x2E: ABS();   ROL();  break;
        case 0x2F: ABS();   RLA();  break;
        case 0x30: REL();   BMI();  break;
        case 0x31: INDY();  AND();  break;
        case 0x32: IMP();   STP();  break;
        case 0x33: INDY();  RLA();  break;
        case 0x34: ZPX();   DOP();  break;
        case 0x35: ZPX();   AND();  break;
        case 0x36: ZPX();   ROL();  break;
        case 0x37: ZPX();   RLA();  break;
        case 0x38: IMP();   SEC();  break;
        case 0x39: ABSY();  AND();  break;
        case 0x3A: IMP();   NOP();  break;
        case 0x3B: ABSY();  RLA();  break;
        case 0x3C: ABSX();  TOP();  break;
        case 0x3D: ABSX();  AND();  break;
        case 0x3E: ABSX();  ROL();  break;
        case 0x3F: ABSX();  RLA();  break;
        case 0x40: IMP();   RTI();  break;
        case 0x41: INDX();  EOR();  break;
        case 0x42: IMP();   STP();  break;
        case 0x43: INDX();  SRE();  break;
        case 0x44: ZP();    DOP();  break;
        case 0x45: ZP();    EOR();  break;
        case 0x46: ZP();    LSR();  break;
        case 0x47: ZP();    SRE();  break;
        case 0x48: IMP();   PHA();  break;
        case 0x49: IMM();   EOR();  break;
        case 0x4A: ACC();   LSRA(); break;
        case 0x4C: ABS();   JMP();  break;
        case 0x4D: ABS();   EOR();  break;
        case 0x4E: ABS();   LSR();  break;
        case 0x4F: ABS();   SRE();  break;
        case 0x50: REL();   BVC();  break;
        case 0x51: INDY();  EOR();  break;
        case 0x52: IMP();   STP();  break;
        case 0x53: INDY();  SRE();  break;
        case 0x54: ZPX();   DOP();  break;
        case 0x55: ZPX();   EOR();  break;
        case 0x56: ZPX();   LSR();  break;
        case 0x57: ZPX();   SRE();  break;
        case 0x59: ABSY();  EOR();  break;
        case 0x5A: IMP();   NOP();  break;
        case 0x5B: ABSY();  SRE();  break;
        case 0x5C: ABSX();  TOP();  break;
        case 0x5D: ABSX();  EOR();  break;
        case 0x5E: ABSX();  LSR();  break;
        case 0x5F: ABSX();  SRE();  break;
        case 0x60: IMP();   RTS();  break;
        case 0x61: INDX();  ADC();  break;
        case 0x62: IMP();   STP();  break;
        case 0x63: INDX();  RRA();  break;
        case 0x64: ZP();    DOP();  break;
        case 0x65: ZP();    ADC();  break;
        case 0x66: ZP();    ROR();  break;
        case 0x67: ZP();    RRA();  break;
        case 0x68: IMP();   PLA();  break;
        case 0x69: IMM();   ADC();  break;
        case 0x6A: ACC();   RORA(); break;
        case 0x6C: IND();   JMP();  break;
        case 0x6D: ABS();   ADC();  break;
        case 0x6E: ABS();   ROR();  break;
        case 0x6F: ABS();   RRA();  break;
        case 0x70: REL();   BVS();  break;
        case 0x71: INDY();  ADC();  break;
        case 0x72: IMP();   STP();  break;
        case 0x73: INDY();  RRA();  break;
        case 0x74: ZPX();   DOP();  break;
        case 0x75: ZPX();   ADC();  break;
        case 0x76: ZPX();   ROR();  break;
        case 0x77: ZPX();   RRA();  break;
        case 0x78: IMP();   SEI();  break;
        case 0x79: ABSY();  ADC();  break;
        case 0x7A: IMP();   NOP();  break;
        case 0x7B: ABSY();  RRA();  break;
        case 0x7C: ABSX();  TOP();  break;
        case 0x7D: ABSX();  ADC();  break;
        case 0x7E: ABSX();  ROR();  break;
        case 0x7F: ABSX();  RRA();  break;
        case 0x80: IMM();   DOP();  break;
        case 0x81: INDX();  STA();  break;
        case 0x82: IMM();   DOP();  break;
        case 0x83: INDX();  SAX();  break;
        case 0x84: ZP();    STY();  break;
        case 0x85: ZP();    STA();  break;
        case 0x86: ZP();    STX();  break;
        case 0x87: ZP();    SAX();  break;
        case 0x88: IMP();   DEY();  break;
        case 0x89: IMM();   DOP();  break;
        case 0x8A: IMP();   TXA();  break;
        case 0x8C: ABS();   STY();  break;
        case 0x8D: ABS();   STA();  break;
        case 0x8E: ABS();   STX();  break;
        case 0x8F: ABS();   SAX();  break;
        case 0x90: REL();   BCC();  break;
        case 0x91: INDY();  STA();  break;
        case 0x92: IMP();   STP();  break;
        case 0x94: ZPX();   STY();  break;
        case 0x95: ZPX();   STA();  break;
        case 0x96: ZPY();   STX();  break;
        case 0x97: ZPY();   SAX();  break;
        case 0x98: IMP();   TYA();  break;
        case 0x99: ABSY();  STA();  break;
        case 0x9A: IMP();   TXS();  break;
        case 0x9D: ABSX();  STA();  break;
        case 0xA0: IMM();   LDY();  break;
        case 0xA1: INDX();  LDA();  break;
        case 0xA2: IMM();   LDX();  break;
        case 0xA3: INDX();  LAX();  break;
        case 0xA4: ZP();    LDY();  break;
        case 0xA5: ZP();    LDA();  break;
        case 0xA6: ZP();    LDX();  break;
        case 0xA7: ZP();    LAX();  break;
        case 0xA8: IMP();   TAY();  break;
        case 0xA9: IMM();   LDA();  break;
        case 0xAA: IMP();   TAX();  break;
        case 0xAC: ABS();   LDY();  break;
        case 0xAD: ABS();   LDA();  break;
        case 0xAE: ABS();   LDX();  break;
        case 0xAF: ABS();   LAX();  break;
        case 0xB0: REL();   BCS();  break;
        case 0xB1: INDY();  LDA();  break;
        case 0xB2: IMP();   STP();  break;
        case 0xB3: INDY();  LAX();  break;
        case 0xB4: ZPX();   LDY();  break;
        case 0xB5: ZPX();   LDA();  break;
        case 0xB6: ZPY();   LDX();  break;
        case 0xB7: ZPY();   LAX();  break;
        case 0xB8: IMP();   CLV();  break;
        case 0xB9: ABSY();  LDA();  break;
        case 0xBA: IMP();   TSX();  break;
        case 0xBC: ABSX();  LDY();  break;
        case 0xBD: ABSX();  LDA();  break;
        case 0xBE: ABSY();  LDX();  break;
        case 0xBF: ABSY();  LAX();  break;
        case 0xC0: IMM();   CPY();  break;
        case 0xC1: INDX();  CMP();  break;
        case 0xC2: IMM();   DOP();  break;
        case 0xC3: INDX();  DCP();  break;
        case 0xC4: ZP();    CPY();  break;
        case 0xC5: ZP();    CMP();  break;
        case 0xC6: ZP();    DEC();  break;
        case 0xC7: ZP();    DCP();  break;
        case 0xC8: IMP();   INY();  break;
        case 0xC9: IMM();   CMP();  break;
        case 0xCA: IMP();   DEX();  break;
        case 0xCC: ABS();   CPY();  break;
        case 0xCD: ABS();   CMP();  break;
        case 0xCE: ABS();   DEC();  break;
        case 0xCF: ABS();   DCP();  break;
        case 0xD0: REL();   BNE();  break;
        case 0xD1: INDY();  CMP();  break;
        case 0xD2: IMP();   STP();  break;
        case 0xD3: INDY();  DCP();  break;
        case 0xD4: ZPX();   DOP();  break;
        case 0xD5: ZPX();   CMP();  break;
        case 0xD6: ZPX();   DEC();  break;
        case 0xD7: ZPX();   DCP();  break;
        case 0xD8: IMP();   CLD();  break;
        case 0xD9: ABSY();  CMP();  break;
        case 0xDA: IMP();   NOP();  break;
        case 0xDB: ABSY();  DCP();  break;
        case 0xDC: ABSX();  TOP();  break;
        case 0xDD: ABSX();  CMP();  break;
        case 0xDE: ABSX();  DEC();  break;
        case 0xDF: ABSX();  DCP();  break;
        case 0xE0: IMM();   CPX();  break;
        case 0xE1: INDX();  SBC();  break;
        case 0xE2: IMM();   DOP();  break;
        case 0xE3: INDX();  ISC();  break;
        case 0xE4: ZP();    CPX();  break;
        case 0xE5: ZP();    SBC();  break;
        case 0xE6: ZP();    INC();  break;
        case 0xE7: ZP();    ISC();  break;
        case 0xE8: IMP();   INX();  break;
        case 0xE9: IMM();   SBC();  break;
        case 0xEA: IMP();   NOP();  break;
        case 0xEB: IMM();   SBC();  break;
        case 0xEC: ABS();   CPX();  break;
        case 0xED: ABS();   SBC();  break;
        case 0xEE: ABS();   INC();  break;
        case 0xEF: ABS();   ISC();  break;
        case 0xF0: REL();   BEQ();  break;
        case 0xF1: INDY();  SBC();  break;
        case 0xF2: IMP();   STP();  break;
        case 0xF3: INDY();  ISC();  break;
        case 0xF4: ZPX();   DOP();  break;
        case 0xF5: ZPX();   SBC();  break;
        case 0xF6: ZPX();   INC();  break;
        case 0xF7: ZPX();   ISC();  break;
        case 0xF8: IMP();   SED();  break;
        case 0xF9: ABSY();  SBC();  break;
        case 0xFA: IMP();   NOP();  break;
        case 0xFB: ABSY();  ISC();  break;
        case 0xFC: ABSX();  TOP();  break;
        case 0xFD: ABSX();  SBC();  break;
        case 0xFE: ABSX();  INC();  break;
        case 0xFF: ABSX();  ISC();  break;
    }

    nes->cpu.cycles += CYCLE_PAIR_TABLE[opcode].c;
}
