#include <cpu/exec.h>
#include "../local-include/rtl.h"
#include <cpu/cpu.h>
#include <cpu/difftest.h>
#include <cpu/dccache.h>

//#define PERF

#include "decode.h"

uint32_t isa_execute(uint32_t n) {
  def_jmp_table();
#ifdef PERF
  static uint64_t instr = 0;
  static uint64_t bp_miss = 0;
  static uint64_t dc_miss = 0;
#endif

  DecodeExecState *s = &dccache[0];
  vaddr_t lpc = cpu.pc; // local pc
  while (true) {
    DecodeExecState *prev = s;
    s ++; // first try sequential fetch with the lowest cost
    if (unlikely(s->pc != lpc)) {
      // if the last instruction is a branch, or `s` is pointing to the sentinel,
      // then try the prediction result
      s = prev->next;
      if (unlikely(s->pc != lpc)) {
        // if the prediction is wrong, re-fetch the correct decode information,
        // and update the prediction
        s = dccache_fetch(lpc);
        prev->next = s;
#ifdef PERF
    bp_miss ++;
#endif
        if (unlikely(s->pc != lpc)) {
          // if it is a miss in decode cache, fetch and decode the correct instruction
          s->pc = lpc;
          fetch_decode(s, jmp_table);
#ifdef PERF
    dc_miss ++;
#endif
        }
      }
    }

#ifdef PERF
    instr ++;
    if (instr % (65536 * 1024) == 0)
      Log("instr = %ld, bp_miss = %ld, dc_miss = %ld", instr, bp_miss, dc_miss);
#endif

    if (--n == 0) break;

    word_t thispc = lpc;
    lpc += 4;
    Operand ldest = { .preg = id_dest->preg };
    Operand lsrc1 = { .preg = id_src1->preg };
    Operand lsrc2 = { .preg = id_src2->preg };

    goto *(s->EHelper);

#include "all-instr.h"
    def_finish();
    IFDEF(CONFIG_DEBUG, debug_hook(s->pc, s->logbuf));
    IFDEF(CONFIG_DIFFTEST, update_gpc(lpc));
    IFDEF(CONFIG_DIFFTEST, difftest_step(s->pc, lpc));
  }
  cpu.pc = lpc;
  return n;
}
