#ifndef __RISCV64_DIFF_TEST_H__
#define __RISCV64_DIFF_TEST_H__

#define DIFFTEST_REG_SIZE (sizeof(uint64_t) * (32 + 1 + 6)) // GRPs + pc + [m|s][status|cause|epc]

void isa_difftest_getregs_hook(void);
void isa_difftest_setregs_hook(void);

#endif
