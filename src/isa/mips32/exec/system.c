#include "cpu/exec.h"
#include "isa/intr.h"

make_EHelper(syscall) {
  void raise_intr(uint32_t, vaddr_t);
  raise_intr(EX_SYSCALL, cpu.pc);

  print_asm("syscall");
}

make_EHelper(eret) {
  rtl_li(&s0, cpu.epc);
  rtl_jr(&s0);
  cpu.status.exl = 0;

  print_asm("eret");
}

make_EHelper(mfc0) {
  uint32_t val;
  switch (id_dest->reg) {
    case 0: val = cpu.index; print_asm("mfc0 %s, index", id_src2->str); break;
    case 10: val = cpu.entryhi.val; print_asm("mfc0 %s, entryhi", id_src2->str); break;
    case 12: val = cpu.status.val; print_asm("mfc0 %s, status", id_src2->str); break;
    case 13: val = cpu.cause; print_asm("mfc0 %s, cause", id_src2->str);
#if defined(DIFF_TEST)
             // qemu may set cause.IP[7]
             difftest_skip_ref();
#endif
             break;
    case 14: val = cpu.epc; print_asm("mfc0 %s, epc", id_src2->str); break;
    default: assert(0);
  }

  rtl_li(&s0, val);
  rtl_sr(id_src2->reg, &s0, 4);
}

make_EHelper(mtc0) {
  switch (id_dest->reg) {
    case 0: cpu.index = id_src2->val; print_asm("mtc0 %s, index", id_src2->str); break;
    case 2: cpu.entrylo0 = id_src2->val; print_asm("mtc0 %s, entrylo0", id_src2->str); break;
    case 3: cpu.entrylo1 = id_src2->val; print_asm("mtc0 %s, entrylo1", id_src2->str); break;
    case 10: cpu.entryhi.val = id_src2->val & ~0x1f00; print_asm("mtc0 %s, entryhi", id_src2->str); break;
    case 12: cpu.status.val = id_src2->val; print_asm("mtc0 %s, status", id_src2->str); break;
    case 13: cpu.cause = id_src2->val; print_asm("mtc0 %s, cause", id_src2->str); break;
    case 14: cpu.epc = id_src2->val; print_asm("mtc0 %s, epc", id_src2->str); break;
    default: assert(0);
  }
}

make_EHelper(tlbwr) {
  extern void tlbwr(void);
  tlbwr();
}

make_EHelper(tlbwi) {
  extern void tlbwi(void);
  tlbwi();
}

make_EHelper(tlbp) {
  extern void tlbp(void);
  tlbp();
}
