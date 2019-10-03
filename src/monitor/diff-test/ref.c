#include "nemu.h"
#include "monitor/diff-test.h"
#include "isa/diff-test.h"
#include "cpu/exec.h"

void cpu_exec(uint64_t);

void difftest_memcpy_from_dut(paddr_t dest, void *src, size_t n) {
  memcpy(guest_to_host(dest), src, n);
}

void difftest_getregs(void *r) {
  memcpy(r, &cpu, DIFFTEST_REG_SIZE);
}

void difftest_setregs(const void *r) {
  memcpy(&cpu, r, DIFFTEST_REG_SIZE);
}

void difftest_exec(uint64_t n) {
  cpu_exec(n);
}

void difftest_raise_intr(word_t NO) {
  void raise_intr(word_t NO, vaddr_t epc);
  raise_intr(NO, cpu.pc);
  update_pc();
}

void difftest_init(void) {
  /* Perform ISA dependent initialization. */
  void init_isa();
  init_isa();
}
