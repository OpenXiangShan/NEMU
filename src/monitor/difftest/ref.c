#include <isa.h>
#include <memory/paddr.h>
#include <cpu/exec.h>
#include <device/map.h>

void cpu_exec(uint64_t);

void difftest_memcpy_from_dut(paddr_t dest, void *src, size_t n) {
  memcpy(guest_to_host(dest - PMEM_BASE), src, n);
}

void difftest_memcpy_from_ref(void *dest, paddr_t src, size_t n) {
  memcpy(dest, guest_to_host(src - PMEM_BASE), n);
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

void difftest_exec(uint64_t n) {
  cpu_exec(n);
}

void difftest_raise_intr(word_t NO) {
  isa_difftest_raise_intr(NO);
}

void difftest_init(void) {
  /* Perform ISA dependent initialization. */
  init_isa();

  /* create dummy address space for serial */
  add_mmio_map("difftest.serial", 0xa10003f8, new_space(8), 8, NULL);
}
