#ifndef __DIFF_TEST_H__
#define __DIFF_TEST_H__

#include "isa/diff-test.h"

void difftest_skip_ref(void);
void difftest_skip_dut(void);

extern void (*ref_difftest_memcpy_from_dut)(paddr_t dest, void *src, size_t n);
extern void (*ref_difftest_getregs)(void *c);
extern void (*ref_difftest_setregs)(const void *c);
extern void (*ref_difftest_exec)(uint64_t n);

static inline bool difftest_check_reg(const char *name, vaddr_t pc, uint32_t ref, uint32_t dut) {
  if (ref != dut) {
    Log("%s is different after executing instruction at pc = 0x%08x, right = 0x%08x, wrong = 0x%08x",
        name, pc, ref, dut);
    return false;
  }
  return true;
}

#endif
