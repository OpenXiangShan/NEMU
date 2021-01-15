#ifndef __DIFFTEST_H__
#define __DIFFTEST_H__

#include <stdint.h>

enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };

#if defined(__ISA_x86__)
# define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 9) // GPRs + PC
#elif defined(__ISA_mips32__)
# define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 38) // GRPs + status + lo + hi + badvaddr + cause + pc
#elif defined(__ISA_riscv32__)
# define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 33) // GRPs + pc
#elif defined(__ISA_riscv64__)
# define DIFFTEST_REG_SIZE (sizeof(uint64_t) * (32 + 1 + 6)) // GRPs + pc + [m|s][status|cause|epc]
#else
# error Unsupport ISA
#endif

#endif
