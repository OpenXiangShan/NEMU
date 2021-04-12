#ifndef __MIPS32_REG_H__
#define __MIPS32_REG_H__

#include <common.h>

enum { PRIV_ERET, PRIV_TLBWR, PRIV_TLBWI, PRIV_TLBP };

static inline int check_reg_index(int index) {
  IFDEF(CONFIG_RT_CHECK, assert(index >= 0 && index < 32));
  return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._32)

static inline const char* reg_name(int index, int width) {
  extern const char* regsl[];
  IFDEF(CONFIG_RT_CHECK, assert(index >= 0 && index < 32));
  return regsl[index];
}

static inline const char* cp0_name(int index) {
  extern const char* cp0[];
  IFDEF(CONFIG_RT_CHECK, assert(index >= 0 && index < 32));
  return cp0[index];
}

#endif
