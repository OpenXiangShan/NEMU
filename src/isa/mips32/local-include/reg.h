#ifndef __MIPS32_REG_H__
#define __MIPS32_REG_H__

#include <common.h>

enum { PRIV_ERET, PRIV_TLBWR, PRIV_TLBWI, PRIV_TLBP };

static inline int check_reg_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < 32));
  return idx;
}

#define gpr(idx) (cpu.gpr[check_reg_idx(idx)]._32)

static inline const char* reg_name(int idx, int width) {
  extern const char* regs[];
  return regs[check_reg_idx(idx)];
}

static inline const char* cp0_name(int idx) {
  extern const char* cp0[];
  return cp0[check_reg_idx(idx)];
}

#endif
