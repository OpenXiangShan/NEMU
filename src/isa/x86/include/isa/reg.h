#ifndef __X86_REG_H__
#define __X86_REG_H__

#include <isa/x86.h>

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
