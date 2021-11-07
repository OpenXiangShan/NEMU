#include <isa.h>
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

#ifndef __ICS_EXPORT
#include "local-include/csr.h"

const char *fpregs[] = {
  "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",
  "fs0", "fs1", "fa0", "fa1", "fa2", "fa3", "fa4", "fa5",
  "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
  "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"
};

void isa_reg_display() {
  int i;
  for (i = 0; i < 32; i ++) {
    printf("%4s: " FMT_WORD " ", regs[i], cpu.gpr[i]);
    if (i % 4 == 3) {
      printf("\n");
    }
  }
  for (i = 0; i < 32; i ++) {
    printf("%4s: " FMT_WORD " ", fpregs[i], cpu.fpr[i]);
    if (i % 4 == 3) {
      printf("\n");
    }
  }
#ifdef CONFIG_MODE_SYSTEM
  printf("pc: " FMT_WORD " mstatus: " FMT_WORD " mcause: " FMT_WORD " mepc: " FMT_WORD "\n",
      cpu.pc, mstatus->val, mcause->val, mepc->val);
  printf("mode: %-16d sstatus: " FMT_WORD " scause: " FMT_WORD " sepc: " FMT_WORD "\n",
      cpu.mode, csrid_read(0x100), scause->val, sepc->val);
#else
  printf("pc: " FMT_WORD "\n", cpu.pc);
#endif
}

word_t isa_reg_str2val(const char *s, bool *success) {
  int i;
  *success = true;
  for (i = 0; i < 32; i ++) {
    if (strcmp(regs[i], s) == 0) return gpr(i);
  }

  if (strcmp("pc", s) == 0) return cpu.pc;

  *success = false;
  return 0;
}
#else
void isa_reg_display() {
}

word_t isa_reg_str2val(const char *s, bool *success) {
  return 0;
}
#endif
