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

uint32_t isa_execute(uint32_t n) {
  for (; n > 0; n --) {
    vaddr_t pc = cpu.pc;
    DecodeExecState *s = dccache_fetch(pc);
    if (s->pc == pc) {
      s->npc = s->snpc;
      goto_EHelper(s->EHelper);
    }

    s->pc = pc;

    /* Execute one instruction, including instruction fetch,
     * instruction decode, and the actual execution. */
#include "all-instr.h"

exec_finish:
    update_pc(s);

    reset_zero();

    cpu_exec_2nd_part(pc, s->snpc, s->npc);
  }
  return n;
}
