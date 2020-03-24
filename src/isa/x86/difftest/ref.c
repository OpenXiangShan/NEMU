#include <isa.h>
#include <cpu/exec.h>
#include "difftest.h"

void isa_difftest_getregs(void *r) {
  memcpy(r, &cpu, DIFFTEST_REG_SIZE);
}

void isa_difftest_setregs(const void *r) {
  memcpy(&cpu, r, DIFFTEST_REG_SIZE);
}

void isa_difftest_raise_intr(word_t NO) {
  DecodeExecState s;
  s.is_jmp = 0;
  s.isa = (ISADecodeInfo) { 0 };

  void raise_intr(DecodeExecState *s, word_t NO, vaddr_t epc);
  raise_intr(&s, NO, cpu.pc);
  update_pc(&s);
}
