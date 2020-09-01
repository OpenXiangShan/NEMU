#include <cpu/exec.h>
#include <monitor/monitor.h>
#include <monitor/difftest.h>

def_EHelper(inv) {
  /* invalid opcode */

  uint32_t instr[2];
  s->seq_pc = cpu.pc;
  instr[0] = instr_fetch(&s->seq_pc, 4);
  instr[1] = instr_fetch(&s->seq_pc, 4);

  printf("invalid opcode(PC = 0x%08x): %08x %08x ...\n\n",
      cpu.pc, instr[0], instr[1]);

  display_inv_msg(cpu.pc);

  rtl_exit(NEMU_ABORT, cpu.pc, -1);

  print_asm("invalid opcode");
}

def_EHelper(nemu_trap) {
  difftest_skip_ref();

#ifdef __ICS_EXPORT
  rtl_exit(NEMU_END, cpu.pc, cpu.gpr[2]._32); // grp[2] is $v0
#else
  // get the latest value for SDI
  rtl_addi(s, s0, &cpu.gpr[2]._32, 0); // grp[2] is $v0
  rtl_exit(NEMU_END, cpu.pc, *s0); 
#endif

  print_asm("nemu trap");
  return;
}
