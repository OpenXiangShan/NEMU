#include <isa.h>
#include <memory/paddr.h>
#include <cpu/exec.h>
#include <device/map.h>

void cpu_exec(uint64_t);

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

void difftest_memcpy_from_dut(paddr_t dest, void *src, size_t n) {
  nemu_large_memcpy(guest_to_host(dest - PMEM_BASE), src, n);
}

void difftest_memcpy_from_ref(void *dest, paddr_t src, size_t n) {
  nemu_large_memcpy(dest, guest_to_host(src - PMEM_BASE), n);
}

void difftest_getregs(void *r) {
  isa_difftest_getregs(r);
}

void difftest_setregs(const void *r) {
  isa_difftest_setregs(r);
}

void difftest_get_mastatus(void *s){
  isa_difftest_get_mastatus(s);
}

void difftest_set_mastatus(const void *s){
  isa_difftest_set_mastatus(s);
}

void difftest_get_csr(void *c){
  isa_difftest_get_csr(c);
}

void difftest_set_csr(const void *c){
  isa_difftest_set_csr(c);
}

vaddr_t disambiguate_exec(void *disambiguate_para){
  return isa_disambiguate_exec(disambiguate_para);
}

int difftest_store_commit(uint64_t *saddr, uint64_t *sdata, uint8_t *smask) {
#ifdef DIFFTEST_STORE_COMMIT
  return check_store_commit(saddr, sdata, smask);
#else
  return 0;
#endif
}

void difftest_exec(uint64_t n) {
  cpu_exec(n);
}

void difftest_raise_intr(word_t NO) {
  isa_difftest_raise_intr(NO);
}

void difftest_init(void) {
  init_mem();

  /* Perform ISA dependent initialization. */
  init_isa();

  /* create dummy address space for serial */
  // add_mmio_map("difftest.serial", 0xa10003f8, new_space(8), 8, NULL);
}
