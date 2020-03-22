#include <isa.h>
#include <monitor/difftest.h>
#include "../local-include/reg.h"
#include "difftest.h"

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  if (memcmp(&cpu.gpr[1], &ref_r->gpr[1], DIFFTEST_REG_SIZE - sizeof(cpu.gpr[0]))) {
    int i;
    // do not check $0
    for (i = 1; i < sizeof(cpu.gpr) / sizeof(cpu.gpr[0]); i ++) {
      difftest_check_reg(reg_name(i, 4), pc, ref_r->gpr[i]._32, cpu.gpr[i]._32);
    }
    difftest_check_reg("pc", pc, ref_r->pc, cpu.pc);
    return false;
  }
  return true;
}

void isa_difftest_attach(void) {
}
