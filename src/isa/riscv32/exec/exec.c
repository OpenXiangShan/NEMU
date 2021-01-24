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

static inline void dcache_add(vaddr_t pc, const void *EHelper, DecodeExecState *s) {
  int idx = get_idx(pc);
  dcache[idx].tag = pc;
  dcache[idx].EHelper = EHelper;
  dcache[idx].s = *s;
}

vaddr_t isa_exec_once() {
  DecodeExecState state;
  DecodeExecState *s;
  int idx = get_idx(cpu.pc);
  int hit = dcache[idx].tag == cpu.pc;
  if (hit) {
    s = &dcache[idx].s;
    s->is_jmp = 0;
    goto *dcache[idx].EHelper;
  }

  s = &state;
  s->is_jmp = 0;
  s->seq_pc = cpu.pc;

  s->isa.instr.val = instr_fetch(&s->seq_pc, 4);
  const void *last_helper = NULL;

#include "all-instr.h"

exec_finish:
  if (!hit) {
    assert(last_helper != NULL);
    dcache_add(cpu.pc, last_helper, s);
  }

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
