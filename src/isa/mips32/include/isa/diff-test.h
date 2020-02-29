#ifndef __MIPS32_DIFF_TEST_H__
#define __MIPS32_DIFF_TEST_H__

#define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 38) // GRPs + status + lo + hi + badvaddr + cause + pc

#define isa_difftest_getregs_hook()
#define isa_difftest_setregs_hook()

#endif
