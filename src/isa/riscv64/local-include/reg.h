#ifndef __RISCV64_REG_H__
#define __RISCV64_REG_H__

#include <common.h>

static inline int check_reg_index(int index) {
  IFDEF(CONFIG_RT_CHECK, assert(index >= 0 && index < 32));
  return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._64)

static inline const char* reg_name(int index, int width) {
  extern const char* regs[];
  IFDEF(CONFIG_RT_CHECK, assert(index >= 0 && index < 32));
  return regs[index];
}

#ifndef __ICS_EXPORT
// Floating Point Regs
#define fpreg_l(index) (cpu.fpr[check_reg_index(index)]._64)

static inline const char* fpreg_name(int index, int width){
  extern const char* fpregs[];
  IFDEF(CONFIG_RT_CHECK, assert(index >= 0 && index < 32));
  return fpregs[index];
}
#endif
#endif
