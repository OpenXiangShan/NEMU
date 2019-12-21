#include "nemu.h"
#include "monitor/diff-test.h"
#include "csr.h"

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  int i;
  for (i = 0; i < sizeof(cpu.gpr) / sizeof(cpu.gpr[0]); i ++) {
    if(!difftest_check_reg(reg_name(i, 4), pc, ref_r->gpr[i]._64, cpu.gpr[i]._64))
      return false;
  }
  return difftest_check_reg("pc", pc, ref_r->pc, cpu.pc);
}

void isa_difftest_attach(void) {
  ref_difftest_memcpy_from_dut(0, guest_to_host(0), PMEM_SIZE);
  ref_difftest_setregs(&cpu);
}

void isa_difftest_getregs_hook(void) {
  cpu.mstatus = mstatus->val;
  cpu.mcause  = mcause->val;
  cpu.mepc    = mepc->val;

  rtlreg_t temp;
  csr_read(&temp, 0x100); // sstatus
  cpu.sstatus = temp;
  cpu.scause  = scause->val;
  cpu.sepc    = sepc->val;
}

void isa_difftest_setregs_hook(void) {
  mstatus->val = cpu.mstatus;
  mcause ->val = cpu.mcause ;
  mepc   ->val = cpu.mepc   ;
  //sstatus->val = cpu.sstatus;  // sstatus is a shadow of mstatus
  scause ->val = cpu.scause ;
  sepc   ->val = cpu.sepc   ;
}
