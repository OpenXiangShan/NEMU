#include <isa.h>
#include <memory/paddr.h>
#include <cpu/difftest.h>
#include "../local-include/reg.h"
#include <difftest.h>

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  if (memcmp(&cpu.gpr[1], &ref_r->gpr[1], DIFFTEST_REG_SIZE - sizeof(cpu.gpr[0]))) {
    int i;
    // do not check $0
    for (i = 1; i < ARRLEN(cpu.gpr); i ++) {
      difftest_check_reg(reg_name(i, 4), pc, ref_r->gpr[i]._64, cpu.gpr[i]._64);
    }
    difftest_check_reg("pc", pc, ref_r->pc, cpu.pc);
    return false;
  }
  return true;
}

void isa_difftest_attach() {
  ref_difftest_memcpy(CONFIG_MBASE, guest_to_host(CONFIG_MBASE), CONFIG_MSIZE, DIFFTEST_TO_REF);
  ref_difftest_regcpy(&cpu, DIFFTEST_TO_REF);
}
