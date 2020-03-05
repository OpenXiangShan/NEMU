#include <isa.h>
#include <monitor/difftest.h>
#include "../local-include/reg.h"
#include "difftest.h"

#define check_reg(r) same = difftest_check_reg(str(r), pc, ref_r->r, cpu.r) && same

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  bool same = true;
  if (memcmp(&cpu, ref_r, sizeof(cpu.gpr))) {
    int i;
    for (i = 0; i < sizeof(cpu.gpr) / sizeof(cpu.gpr[0]); i ++) {
      difftest_check_reg(reg_name(i, 4), pc, ref_r->gpr[i]._32, cpu.gpr[i]._32);
    }
    same = false;
  }
  check_reg(pc);
  check_reg(lo);
  check_reg(hi);

  return same;
}

void isa_difftest_attach(void) {
}
