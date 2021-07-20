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

  #ifdef CONFIG_RVV_010
  cpu.vtype   = vtype->val;
  cpu.vstart  = vstart->val;
  cpu.vxsat   = vxsat->val;
  cpu.vxrm    = vxrm->val;
  cpu.vl      = vl->val;
  #endif // CONFIG_RVV_010
}

static void csr_writeback() {
   return;
  mstatus->val = cpu.mstatus;
  mcause ->val = cpu.mcause ;
  mepc   ->val = cpu.mepc   ;
  //sstatus->val = cpu.sstatus;  // sstatus is a shadow of mstatus
  scause ->val = cpu.scause ;
  sepc   ->val = cpu.sepc   ;

  #ifdef CONFIG_RVV_010
  vtype->val   = cpu.vtype;
  vstart->val  = cpu.vstart;
  vxsat->val   = cpu.vxsat;
  vxrm->val    = cpu.vxrm;
  vl->val      = cpu.vl;
  #endif //CONFIG_RVV_010
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
