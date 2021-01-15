#include <isa.h>
#include <cpu/exec.h>
#include <difftest.h>

void isa_difftest_regcpy(void *dut, bool direction) {
  if (direction == DIFFTEST_TO_REF) memcpy(&cpu, dut, DIFFTEST_REG_SIZE);
  else memcpy(dut, &cpu, DIFFTEST_REG_SIZE);
}

void isa_difftest_raise_intr(word_t NO) {
  DecodeExecState s;
  s.is_jmp = 0;
  s.isa = (ISADecodeInfo) { 0 };

  void raise_intr(DecodeExecState *s, word_t NO, vaddr_t epc);
  raise_intr(&s, NO, cpu.pc);
  update_pc(&s);
}
