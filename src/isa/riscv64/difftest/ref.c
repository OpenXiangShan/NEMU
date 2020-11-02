#include <isa.h>
#include <cpu/exec.h>
#include "../local-include/csr.h"
#include "difftest.h"

static void csr_prepare() {
  // return;
  cpu.mstatus = mstatus->val;
  cpu.mcause  = mcause->val;
  cpu.mepc    = mepc->val;

  rtlreg_t temp;
  csr_read(&temp, 0x100); // sstatus
  cpu.sstatus = temp;
  cpu.scause  = scause->val;
  cpu.sepc    = sepc->val;

  cpu.satp     = satp->val;
  cpu.mip      = mip->val;
  cpu.mie      = mie->val;
  cpu.mscratch = mscratch->val;
  cpu.sscratch = sscratch->val;
  cpu.mideleg  = mideleg->val;
  cpu.medeleg  = medeleg->val;
  cpu.mtval    = mtval->val;
  cpu.stval    = stval->val;
  cpu.mtvec    = mtvec->val;
  cpu.stvec    = stvec->val;
}

static void csr_writeback() {
  // return;
  mstatus->val = cpu.mstatus;
  mcause ->val = cpu.mcause ;
  mepc   ->val = cpu.mepc   ;
  //sstatus->val = cpu.sstatus;  // sstatus is a shadow of mstatus
  scause ->val = cpu.scause ;
  sepc   ->val = cpu.sepc   ;

  satp->val     = cpu.satp;
  mip->val      = cpu.mip;
  mie->val      = cpu.mie;
  mscratch->val = cpu.mscratch;
  sscratch->val = cpu.sscratch;
  mideleg->val  = cpu.mideleg;
  medeleg->val  = cpu.medeleg;
  mtval->val    = cpu.mtval;
  stval->val    = cpu.stval;
  mtvec->val    = cpu.mtvec;
  stvec->val    = cpu.stvec;
}

void isa_difftest_getregs(void *r) {
  csr_prepare();
  memcpy(r, &cpu, DIFFTEST_REG_SIZE);
}

void isa_difftest_setregs(const void *r) {
  memcpy(&cpu, r, DIFFTEST_REG_SIZE);
  csr_writeback();
}

void isa_difftest_get_mastatus(void *s) {
  struct SyncState ms;
  ms.lrscValid = cpu.lr_valid;
  ms.lrscAddr = cpu.lr_addr;
  memcpy(s, &ms, sizeof(struct SyncState));
}

void isa_difftest_set_mastatus(const void *s) {
  struct SyncState* ms = (struct SyncState*)s;
  cpu.lr_valid = ms->lrscValid;
  cpu.lr_addr = ms->lrscAddr;
}

void isa_difftest_set_csr(const void *c) {
  memcpy(csr_array, c, 4096 * sizeof(rtlreg_t));
}

void isa_difftest_get_csr(void *c) {
  memcpy(c, csr_array, 4096 * sizeof(rtlreg_t));
}

void isa_difftest_raise_intr(word_t NO) {
  DecodeExecState s;
  s.is_jmp = 0;
  s.isa = (ISADecodeInfo) { 0 };

  void raise_intr(DecodeExecState *s, word_t NO, vaddr_t epc);
  raise_intr(&s, NO, cpu.pc);
  update_pc(&s);
}
