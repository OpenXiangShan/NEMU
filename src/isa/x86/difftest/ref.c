#include <isa.h>
#include <difftest.h>
#include "../local-include/intr.h"

void isa_difftest_regcpy(void *dut, bool direction) {
  if (direction == DIFFTEST_TO_REF) memcpy(&cpu, dut, DIFFTEST_REG_SIZE);
  else memcpy(dut, &cpu, DIFFTEST_REG_SIZE);
}

void isa_difftest_raise_intr(word_t NO) {
  cpu.pc = raise_intr(NO, cpu.pc);
}
