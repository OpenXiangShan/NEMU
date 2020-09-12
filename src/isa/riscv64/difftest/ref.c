#include <isa.h>
#include <difftest.h>
#include "../local-include/intr.h"
#include "../local-include/csr.h"

static void csr_prepare() {
   return;
  cpu.mstatus = mstatus->val;
  cpu.mcause  = mcause->val;
  cpu.mepc    = mepc->val;

  cpu.sstatus = csrid_read(0x100); // sstatus
  cpu.scause  = scause->val;
  cpu.sepc    = sepc->val;

  cpu.satp     = satp->val;
  cpu.mip      = mip->val;
  cpu.mie      = mie->val;
  cpu.mscratch = mscratch->val;
  cpu.sscratch = sscratch->val;
  cpu.mideleg  = mideleg->val;
  cpu.medeleg  = medeleg->val;
}

static void csr_writeback() {
   return;
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
}

void isa_difftest_regcpy(void *dut, bool direction) {
  if (direction == DIFFTEST_TO_REF) {
    memcpy(&cpu, dut, DIFFTEST_REG_SIZE);
    csr_writeback();
  } else {
    csr_prepare();
    memcpy(dut, &cpu, DIFFTEST_REG_SIZE);
  }
}

void isa_difftest_raise_intr(word_t NO) {
  cpu.pc = raise_intr(NO, cpu.pc);
}
