#include "cpu/exec.h"

CPU_state cpu;

void update_pc(DecodeExecState *s) {
  if (s->is_jmp) { s->is_jmp = 0; }
  else { cpu.pc = s->seq_pc; }
}
