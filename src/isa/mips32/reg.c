#include <isa.h>
#include "local-include/reg.h"

const char *regsl[] = {
  "$0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
  "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"
};

#ifndef __ICS_EXPORT
const char *cp0[] = {
  "index", "???", "entrylo0", "entrylo1",
  "???", "???", "???", "???",
  "???", "entryhi", "???", "status",
  "cause", "epc", "???", "???",
  "???", "???", "???", "???",
  "???", "???", "???", "???",
  "???", "???", "???", "???",
  "???", "???", "???", "???"
};

void isa_reg_display() {
  int i;
  for (i = 0; i < 32; i ++) {
    printf("%s: 0x%08x ", regsl[i], cpu.gpr[i]._32);
    if (i % 4 == 3) printf("\n");
  }
  printf("pc: 0x%08x\n", cpu.pc);
}

word_t isa_reg_str2val(const char *s, bool *success) {
  int i;
  *success = true;
  for (i = 0; i < 32; i ++) {
    if (strcmp(regsl[i], s) == 0) return reg_l(i);
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
