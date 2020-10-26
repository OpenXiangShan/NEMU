#ifndef __X86_REG_H__
#define __X86_REG_H__

#include <isa.h>

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI };
enum { R_AL, R_CL, R_DL, R_BL, R_AH, R_CH, R_DH, R_BH };
enum { SR_ES, SR_CS, SR_SS, SR_DS, SR_FS, SR_GS, SR_TR, SR_LDTR };

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

static inline const char* sreg_name(int index) {
  const char *name[] = { "es", "cs", "ss", "ds", "fs", "gs" };
  assert(index >= 0 && index < sizeof(name) / sizeof(name[0]));
  return name[index];
}

enum { MODE_R0, MODE_R1, MODE_R2, MODE_R3 };

#ifndef __PA__
#define return_on_mem_ex() do { if (cpu.mem_exception != 0) return; } while (0)
#else
#define return_on_mem_ex()
#endif

#endif
