#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "../local-include/rtl.h"
#include <cpu/difftest.h>
#include <cpu/cpu-exec.h>
#include <cpu/dccache.h>

#define goto_EHelper(addr) goto *(addr)
#undef decode_empty
static inline def_DHelper(empty) { }

static inline void reset_zero() { reg_l(0) = 0; }

void isa_execute(uint64_t n) {
  for (; n > 0; n --) {
    vaddr_t pc = cpu.pc;
    int idx = dccache_idx(pc);
    DecodeExecState *s = &dccache[idx].s;
    if (dccache[idx].tag == pc) {
      s->npc = s->snpc;
      goto_EHelper(dccache[idx].EHelper);
    }

    dccache[idx].tag = pc;

    /* Execute one instruction, including instruction fetch,
     * instruction decode, and the actual execution. */
#include "all-instr.h"

exec_finish:
    update_pc(s);

    reset_zero();

    cpu_exec_2nd_part(pc, s->snpc, s->npc);
  }
}
