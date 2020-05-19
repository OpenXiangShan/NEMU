#include "../local-include/intr.h"

static inline make_EHelper(syscall) {
  raise_intr(s, EX_SYSCALL, cpu.pc);
  print_asm("syscall");
}

static inline make_EHelper(eret) {
  rtl_j(s, cpu.epc);
  cpu.status.exl = 0;
  print_asm("eret");
}

static inline make_EHelper(mfc0) {
  uint32_t val;
  switch (id_dest->reg) {
    case 0: val = cpu.index; print_asm("mfc0 %s, index", id_src2->str); break;
    case 10: val = cpu.entryhi.val; print_asm("mfc0 %s, entryhi", id_src2->str); break;
    case 12: val = cpu.status.val; print_asm("mfc0 %s, status", id_src2->str); break;
    case 13: val = cpu.cause; print_asm("mfc0 %s, cause", id_src2->str);
#ifndef __DIFF_REF_NEMU__
             // qemu may set cause.IP[7]
             difftest_skip_ref();
#endif
             break;
    case 14: val = cpu.epc; print_asm("mfc0 %s, epc", id_src2->str); break;
    default: assert(0);
  }

  rtl_li(s, dsrc2, val);
}

static inline make_EHelper(mtc0) {
  switch (id_dest->reg) {
    case 0: cpu.index = *dsrc2; print_asm("mtc0 %s, index", id_src2->str); break;
    case 2: cpu.entrylo0 = *dsrc2; print_asm("mtc0 %s, entrylo0", id_src2->str); break;
    case 3: cpu.entrylo1 = *dsrc2; print_asm("mtc0 %s, entrylo1", id_src2->str); break;
    case 10: cpu.entryhi.val = *dsrc2 & ~0x1f00; print_asm("mtc0 %s, entryhi", id_src2->str); break;
    case 12: cpu.status.val = *dsrc2; print_asm("mtc0 %s, status", id_src2->str); break;
    case 13: cpu.cause = *dsrc2; print_asm("mtc0 %s, cause", id_src2->str); break;
    case 14: cpu.epc = *dsrc2; print_asm("mtc0 %s, epc", id_src2->str); break;
    default: assert(0);
  }
}

static inline make_EHelper(tlbwr) {
  extern void tlbwr(void);
  tlbwr();
}

static inline make_EHelper(tlbwi) {
  extern void tlbwi(void);
  tlbwi();
}

static inline make_EHelper(tlbp) {
  extern void tlbp(void);
  tlbp();
}
