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
  switch (id_dest->reg) {
    case 12: val = cpu.status; print_asm("mfc0 %s, status", id_src2->str); break;
    case 13: val = cpu.cause; print_asm("mfc0 %s, cause", id_src2->str); break;
    case 14: val = cpu.epc; print_asm("mfc0 %s, epc", id_src2->str); break;
    default: assert(0);
  }

  rtl_li(&s0, val);
  rtl_sr(id_src2->reg, &s0, 4);

}

make_EHelper(mtc0) {
#if defined(DIFF_TEST)
  difftest_skip_ref();
#endif

  switch (id_dest->reg) {
    case 12: cpu.status = id_src2->val; print_asm("mtc0 %s, status", id_src2->str); break;
    case 13: cpu.cause = id_src2->val; print_asm("mtc0 %s, cause", id_src2->str); break;
    case 14: cpu.epc = id_src2->val; print_asm("mtc0 %s, epc", id_src2->str); break;
    default: assert(0);
  }

}
