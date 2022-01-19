#include <isa.h>
#include <cpu/cpu.h>
#include <difftest-def.h>
#ifdef CONFIG_MODE_SYSTEM
#include <memory/paddr.h>
#else
#include "../../user/user.h"
#define guest_to_host user_to_host
#endif

void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
  if (direction == DIFFTEST_TO_REF) memcpy(guest_to_host(addr), buf, n);
  else memcpy(buf, guest_to_host(addr), n);
}

void difftest_regcpy(void *dut, bool direction) {
  isa_difftest_regcpy(dut, direction);
}

void difftest_exec(uint64_t n) {
  cpu_exec(n);
}

void difftest_raise_intr(word_t NO) {
  isa_difftest_raise_intr(NO);
}

void difftest_init() {
#ifdef CONFIG_MODE_USER
  void init_mem();
  void isa_init_user(word_t sp);
  init_mem();
  isa_init_user(0);
#else
  /* Perform ISA dependent initialization. */
  init_isa();
#endif
}
