#ifndef __DIFF_TEST_H__
#define __DIFF_TEST_H__

#include "common.h"
#include "isa/diff-test.h"

#ifdef DIFF_TEST
void difftest_skip_ref(void);
void difftest_skip_dut(int nr_ref, int nr_dut);
void difftest_step(vaddr_t ori_pc, vaddr_t next_pc);
#else
#define difftest_skip_ref()
#define difftest_skip_dut(nr_ref, nr_dut)
#define difftest_step(ori_pc, next_pc)
#endif

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
