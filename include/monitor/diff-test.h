#ifndef __DIFF_TEST_H__
#define __DIFF_TEST_H__

#define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 9) // GRPs + EIP

void difftest_skip_ref(void);
void difftest_skip_dut(void);
void difftest_skip_eflags(uint32_t mask);

#endif
