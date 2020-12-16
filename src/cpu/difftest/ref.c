#include <isa.h>
#include <memory/paddr.h>
#include <cpu/cpu.h>
#include <difftest.h>

void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
  if (direction == DIFFTEST_TO_REF) memcpy(guest_to_host(addr), buf, n);
  else memcpy(buf, guest_to_host(addr), n);
}

void difftest_regcpy(void *dut, bool direction) {
  isa_difftest_regcpy(dut, direction);
}

#ifdef RV64_FULL_DIFF
void difftest_csrcpy(void *dut, bool direction) {
  isa_difftest_csrcpy(dut, direction);
}

void difftest_uarchstatus_cpy(void *dut, bool direction) {
  isa_difftest_uarchstatus_cpy(dut, direction);
}
#endif

void difftest_exec(uint64_t n) {
  cpu_exec(n);
}

void difftest_raise_intr(word_t NO) {
  isa_difftest_raise_intr(NO);
}

void difftest_init() {
  init_mem();

  /* Perform ISA dependent initialization. */
  init_isa();
  /* create dummy address space for serial */
  //add_mmio_map("difftest.serial", 0xa10003f8, new_space(8), 8, NULL);
}
