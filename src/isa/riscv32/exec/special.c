#include "cpu/exec.h"
#include "monitor/monitor.h"

make_EHelper(inv) {
  /* invalid opcode */

  uint32_t instr[2];
  vaddr_t ori_pc = cpu.pc;
  *eip = ori_pc;
  instr[0] = instr_fetch(eip, 4);
  instr[1] = instr_fetch(eip, 4);

  printf("invalid opcode(pc = 0x%08x): %08x %08x ...\n\n",
      cpu.pc, instr[0], instr[1]);

  extern char logo [];
  printf("There are two cases which will trigger this unexpected exception:\n"
      "1. The instruction at PC = 0x%08x is not implemented.\n"
      "2. Something is implemented incorrectly.\n", ori_pc);
  printf("Find this PC(0x%08x) in the disassembling result to distinguish which case it is.\n\n", ori_pc);
  printf("\33[1;31mIf it is the first case, see\n%s\nfor more details.\n\nIf it is the second case, remember:\n"
      "* The machine is always right!\n"
      "* Every line of untested code is always wrong!\33[0m\n\n", logo);

  rtl_exit(NEMU_ABORT, cpu.pc, -1);

  print_asm("invalid opcode");
}

make_EHelper(nemu_trap) {
#if defined(DIFF_TEST)
  difftest_skip_ref();
#endif

  rtl_exit(NEMU_END, cpu.pc, cpu.gpr[10]._32); // grp[10] is $a0

  print_asm("nemu trap");
  return;
}
