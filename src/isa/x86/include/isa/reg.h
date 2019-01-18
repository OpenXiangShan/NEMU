#ifndef __X86_REG_H__
#define __X86_REG_H__

#include "common.h"
#include "isa/mmu.h"

#define PC_START 0x100000

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI };
enum { R_AL, R_CL, R_DL, R_BL, R_AH, R_CH, R_DH, R_BH };

#define EFLAGS_BIT_CF 0
#define EFLAGS_BIT_ZF 6
#define EFLAGS_BIT_SF 7
#define EFLAGS_BIT_IF 9
#define EFLAGS_BIT_OF 11

#define _EFLAGS(f) f(OF) f(IF) f(SF) f(ZF) f(CF)
#define __f(flag) concat(EFLAGS_MASK_, flag) = 1 << concat(EFLAGS_BIT_, flag),
enum {
  MAP(_EFLAGS, __f)
#undef __f
#define __f(flag) | concat(EFLAGS_MASK_, flag)
  EFLAGS_MASK_ALL = 0 MAP(_EFLAGS, __f)
#undef __f
};

/* TODO: Re-organize the `CPU_state' structure to match the register
 * encoding scheme in i386 instruction format. For example, if we
 * access cpu.gpr[3]._16, we will get the `bx' register; if we access
 * cpu.gpr[1]._8[1], we will get the 'ch' register. Hint: Use `union'.
 * For more details about the register encoding scheme, see i386 manual.
 */

typedef struct {
  union {
    union {
      uint32_t _32;
      uint16_t _16;
      uint8_t _8[2];
    } gpr[8];

  /* Do NOT change the order of the GPRs' definitions. */

  /* In NEMU, rtlreg_t is exactly uint32_t. This makes RTL instructions
   * in PA2 able to directly access these registers.
   */
    struct {
      rtlreg_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    };
  };

  union {
    vaddr_t pc;
    vaddr_t eip;
  };
  uint32_t eflags;
  uint16_t cs;

  rtlreg_t OF, CF, SF, ZF, IF;

  struct {
    uint32_t limit :16;
    uint32_t base  :32;
  } idtr;

  union {
    rtlreg_t cr[4];
    struct {
      CR0 cr0;
      rtlreg_t cr1;
      rtlreg_t cr2;
      CR3 cr3;
    };
  };

  bool INTR;
} CPU_state;

static inline int check_reg_index(int index) {
  assert(index >= 0 && index < 8);
  return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._32)
#define reg_w(index) (cpu.gpr[check_reg_index(index)]._16)
#define reg_b(index) (cpu.gpr[check_reg_index(index) & 0x3]._8[index >> 2])

static inline const char* reg_name(int index, int width) {
  extern const char* regsl[];
  extern const char* regsw[];
  extern const char* regsb[];
  assert(index >= 0 && index < 8);

  switch (width) {
    case 4: return regsl[index];
    case 1: return regsb[index];
    case 2: return regsw[index];
    default: assert(0);
  }
}

#endif
