#include "cpu/exec.h"

void raise_intr(uint8_t, vaddr_t);
#define EX_SYSCALL 8

make_EHelper(syscall) {
#if defined(DIFF_TEST)
  difftest_skip_ref();
#endif

  raise_intr(EX_SYSCALL, cpu.pc);

  print_asm("syscall");
}

make_EHelper(eret) {
#if defined(DIFF_TEST)
  difftest_skip_ref();
#endif

  rtl_li(&s0, cpu.epc);
  rtl_jr(&s0);

  print_asm("eret");
}

make_EHelper(mfc0) {
#if defined(DIFF_TEST)
  difftest_skip_ref();
#endif

  uint32_t val;
  char *name;
  switch (id_dest->reg) {
    case 12: val = cpu.status; name = "status"; break;
    case 13: val = cpu.cause; name = "cause"; break;
    case 14: val = cpu.epc; name = "epc"; break;
    default: assert(0);
  }

  rtl_li(&s0, val);
  rtl_sr(id_src2->reg, &s0, 4);

  print_asm("mfc0 %s, %s", id_src2->str, name);
}

make_EHelper(mtc0) {
#if defined(DIFF_TEST)
  difftest_skip_ref();
#endif

  char *name;
  switch (id_dest->reg) {
    case 12: cpu.status = id_src2->val; name = "status"; break;
    case 13: cpu.cause = id_src2->val; name = "cause"; break;
    case 14: cpu.epc = id_src2->val; name = "epc"; break;
    default: assert(0);
  }

  print_asm("mtc0 %s, %s", id_src2->str, name);
}
