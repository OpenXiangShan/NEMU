#include <isa.h>
#include <memory/paddr.h>
#include <cpu/exec.h>
#include <device/map.h>

void cpu_exec(uint64_t);

void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool to_ref) {
  if (to_ref) memcpy(guest_to_host(addr - PMEM_BASE), buf, n);
  else memcpy(buf, guest_to_host(addr - PMEM_BASE), n);
}

void difftest_regcpy(void *dut, bool to_ref) {
  isa_difftest_regcpy(dut, to_ref);
}

void difftest_exec(uint64_t n) {
  cpu_exec(n);
}

void difftest_raise_intr(word_t NO) {
  isa_difftest_raise_intr(NO);
}

void difftest_init() {
  /* Perform ISA dependent initialization. */
  init_isa();
  /* create dummy address space for serial */
  add_mmio_map("difftest.serial", 0xa10003f8, new_space(8), 8, NULL);
}
