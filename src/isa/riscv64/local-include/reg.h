#ifndef __RISCV64_REG_H__
#define __RISCV64_REG_H__

#include <common.h>

static inline int check_reg_index(int index) {
  assert(index >= 0 && index < 32);
  return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._64)

static inline const char* reg_name(int index, int width) {
  extern const char* regsl[];
  assert(index >= 0 && index < 32);
  return regsl[index];
}

// Floating Point Regs
#define fpreg_l(index) (cpu.fpr[check_reg_index(index)]._64)

static inline const char* fpreg_name(int index, int width){
  extern const char* fpregsl[];
  assert(index >=0 && index < 32);
  return fpregsl[index];
}

#endif
