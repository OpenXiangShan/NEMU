#include <isa.h>
#include <memory/memory.h>
#include <monitor/difftest.h>
#include "../local-include/reg.h"
#include "difftest.h"

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  if (memcmp(&cpu, ref_r, DIFFTEST_REG_SIZE)) {
    int i;
    for (i = 0; i < sizeof(cpu.gpr) / sizeof(cpu.gpr[0]); i ++) {
      difftest_check_reg(reg_name(i, 4), pc, ref_r->gpr[i]._64, cpu.gpr[i]._64);
    }
    difftest_check_reg("pc", pc, ref_r->pc, cpu.pc);
    return false;
  }
  return true;
}

void isa_difftest_attach(void) {
  ref_difftest_memcpy_from_dut(0, guest_to_host(0), PMEM_SIZE);
  ref_difftest_setregs(&cpu);
}
