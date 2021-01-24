#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "../local-include/rtl.h"
#include <monitor/difftest.h>

#undef CASE_ENTRY
#define CASE_ENTRY(idx, id, ex, w) case idx: id(s); ex(s); break;

static inline void reset_zero() {
  reg_l(0) = 0;
}

vaddr_t isa_exec_once() {
  DecodeExecState state;
  DecodeExecState *const s = &state;
  s->is_jmp = 0;
  s->seq_pc = cpu.pc;

  s->isa.instr.val = instr_fetch(&s->seq_pc, 4);
  if (s->isa.instr.i.opcode1_0 != 0x3) {
    goto inv;
  } else {
#include "all-instr.h"
  }

exec_finish:
  update_pc(s);
#ifndef __ICS_EXPORT

#if !defined(DIFF_TEST) && !_SHARE
  void query_intr();
  query_intr();
#endif
#endif

  reset_zero();

  return s->seq_pc;
}
