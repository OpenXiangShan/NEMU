#include <isa.h>
#include <cpu/exec.h>
#include "../local-include/csr.h"
#include "difftest.h"

static void csr_prepare() {
  cpu.mstatus = mstatus->val;
  cpu.mcause  = mcause->val;
  cpu.mepc    = mepc->val;

  rtlreg_t temp;
  csr_read(&temp, 0x100); // sstatus
  cpu.sstatus = temp;
  cpu.scause  = scause->val;
  cpu.sepc    = sepc->val;
}

static void csr_writeback() {
  mstatus->val = cpu.mstatus;
  mcause ->val = cpu.mcause ;
  mepc   ->val = cpu.mepc   ;
  //sstatus->val = cpu.sstatus;  // sstatus is a shadow of mstatus
  scause ->val = cpu.scause ;
  sepc   ->val = cpu.sepc   ;
}

void isa_difftest_getregs(void *r) {
  csr_prepare();
  memcpy(r, &cpu, DIFFTEST_REG_SIZE);
}

void isa_difftest_setregs(const void *r) {
  memcpy(&cpu, r, DIFFTEST_REG_SIZE);
  csr_writeback();
}

void isa_difftest_raise_intr(word_t NO) {
  DecodeExecState s;
  s.is_jmp = 0;
  s.isa = (ISADecodeInfo) { 0 };

  void raise_intr(DecodeExecState *s, word_t NO, vaddr_t epc);
  raise_intr(&s, NO, cpu.pc);
  update_pc(&s);
}
