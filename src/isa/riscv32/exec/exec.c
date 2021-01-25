#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "../local-include/rtl.h"
#include <cpu/difftest.h>
#include <cpu/cpu-exec.h>

#define goto_EHelper(addr) goto *(addr)
#undef decode_empty
static inline def_DHelper(empty) { }

static inline void reset_zero() { reg_l(0) = 0; }

#define DCACHE_SIZE 256
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

void execute(uint64_t n) {
  for (; n > 0; n --) {
    /* Execute one instruction, including instruction fetch,
     * instruction decode, and the actual execution. */
    vaddr_t this_pc = cpu.pc;
    int idx = get_idx(this_pc);
    DecodeExecState *s = &dcache[idx].s;
    if (dcache[idx].tag == this_pc) {
      s->npc = s->spc;
      goto_EHelper(dcache[idx].EHelper);
    }

    dcache[idx].tag = this_pc;

#include "all-instr.h"

exec_finish:
    update_pc(s);

    reset_zero();

    cpu_exec_2nd_part(this_pc, s->spc, cpu.pc);
  }
}
