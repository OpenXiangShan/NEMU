#include "nemu.h"
#include "monitor/diff-test.h"

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  /*
  int i;
  for (i = 0; i < sizeof(cpu.gpr) / sizeof(cpu.gpr[0]); i ++) {
    if (((uint64_t)cpu.gpr[i]._64) > ((uint64_t)0xffffffff)) {
      printf("!!-0x%lx, 0x%lx\n", ref_r->gpr[i]._64, cpu.gpr[i]._64);
    }
  }*/
  
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
}
