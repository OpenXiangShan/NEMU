#ifndef __RISCV64_DIFF_TEST_H__
#define __RISCV64_DIFF_TEST_H__

#define DIFFTEST_REG_SIZE (sizeof(uint64_t) * (32 + 32 + 1 + 6 + 11 + 1))
// GRPs + FPRs + pc + [m|s][status|cause|epc] + other necessary CSRs + mode

void isa_difftest_getregs_hook(void);
void isa_difftest_setregs_hook(void);

struct SyncState {
  uint64_t lrscValid;
  uint64_t lrscAddr;
};

#endif
