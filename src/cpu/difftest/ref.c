#include <isa.h>
#include <memory/paddr.h>
#include <cpu/cpu.h>
#include <difftest.h>

static void nemu_large_memcpy(void *dest, void *src, size_t n) {
  uint64_t *_dest = (uint64_t *)dest;
  uint64_t *_src  = (uint64_t *)src;
  while (n >= sizeof(uint64_t)) {
    if (*_src != 0) {
      *_dest = *_src;
    }
    _dest++;
    _src++;
    n -= sizeof(uint64_t);
  }
  if (n > 0) {
    uint8_t *dest8 = (uint8_t *)_dest;
    uint8_t *src8  = (uint8_t *)_src;
    while (n > 0) {
      *dest8 = *src8;
      dest8++;
      src8++;
      n--;
    }
  }
}

void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
#ifdef CONFIG_LARGE_COPY
  if (direction == DIFFTEST_TO_REF) nemu_large_memcpy(guest_to_host(addr), buf, n);
  else nemu_large_memcpy(buf, guest_to_host(addr), n);
#else
  if (direction == DIFFTEST_TO_REF) memcpy(guest_to_host(addr), buf, n);
  else memcpy(buf, guest_to_host(addr), n);
#endif
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

int difftest_store_commit(uint64_t *saddr, uint64_t *sdata, uint8_t *smask) {
#ifdef DIFFTEST_STORE_COMMIT
  return check_store_commit(saddr, sdata, smask);
#else
  return 0;
#endif
}

// vaddr_t disambiguate_exec(void *disambiguate_para){
//   return isa_disambiguate_exec(disambiguate_para);
// }
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
