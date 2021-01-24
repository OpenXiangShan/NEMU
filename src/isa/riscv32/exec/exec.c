#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "../local-include/rtl.h"
#include <monitor/difftest.h>

#undef CASE_ENTRY
#define CASE_ENTRY(idx, id, ex, w) case idx: id(s); ex(s); break;

static inline void reset_zero() {
  reg_l(0) = 0;
}

#undef decode_empty
static inline def_DHelper(empty) {
}

#define DCACHE_SIZE 4096
static struct {
  vaddr_t tag;
  const void *EHelper;
  DecodeExecState s;
} dcache[DCACHE_SIZE];

static inline int get_idx(vaddr_t pc) {
  return (pc >> 2) % DCACHE_SIZE;
}

void dcache_flush() {
  int i;
  for (i = 0; i < DCACHE_SIZE; i ++) {
    dcache[i].tag = 0x1;  // set tag to an invalid pc
  }
}

vaddr_t isa_exec_once() {
  int idx = get_idx(cpu.pc);
  DecodeExecState *s = &dcache[idx].s;
  s->is_jmp = 0;
  if (dcache[idx].tag == cpu.pc) {
    goto *dcache[idx].EHelper;
  }

  dcache[idx].tag = cpu.pc;
  s->seq_pc = cpu.pc;
  s->isa.instr.val = instr_fetch(&s->seq_pc, 4);

#include "all-instr.h"

exec_finish:
  update_pc(s);
#ifndef __ICS_EXPORT

#if !defined(DIFF_TEST) && !_SHARE
//  void query_intr();
//  query_intr();
#endif
#endif

  reset_zero();

  return s->seq_pc;
}
