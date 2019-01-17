#include "nemu.h"
#include "monitor/diff-test.h"

void isa_difftest_syncregs() {
  ref_difftest_setregs(&cpu);
}

#define check_reg(r) same = same && difftest_check_reg(str(r), pc, ref_r->r, cpu.r)

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
  check_reg(status);

  return same;
}

void isa_difftest_attach(void) {
}
