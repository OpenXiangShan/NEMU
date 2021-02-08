#include <cpu/exec.h>
#include "../local-include/rtl.h"
#include <cpu/cpu.h>
#include <cpu/difftest.h>
#include <cpu/dccache.h>

#define INSTR_LIST(f) \
  f(lui) f(add) f(sll) f(srl) f(slt) f(sltu) f(xor) f(or) f(sub) f(sra) \
  f(and) f(addi) f(slli) f(srli) f(slti) f(sltui) f(xori) f(ori) f(srai) \
  f(andi) f(auipc) f(jal) f(jalr) f(beq) f(bne) f(blt) f(bge) \
  f(bltu) f(bgeu) f(lw) f(sw) f(lh) f(lb) f(lhu) f(lbu) \
  f(sh) f(sb) f(mul) f(mulh) f(mulhu) f(mulhsu) f(div) f(divu) \
  f(rem) f(remu) f(inv) f(nemu_trap) f(csrrw) f(csrrs) f(ecall) f(sret) f(sfence_vma)

#define def_EXEC_ID(name) \
  enum { concat(EXEC_ID_, name) = __COUNTER__ }; \
  static inline int concat(table_, name) (DecodeExecState *s) { return concat(EXEC_ID_, name); }

MAP(INSTR_LIST, def_EXEC_ID)

#define INSTR_CNT(name) + 1
#define TOTAL_INSTR (0 MAP(INSTR_LIST, INSTR_CNT))

#define FILL_JMP_TABLE(name) [concat(EXEC_ID_, name)] = &&name,

#include "../local-include/decode.h"
#include "table.h"

//#define PERF

uint32_t isa_execute(uint32_t n) {
  static const void* jmp_table[TOTAL_INSTR] = {
    MAP(INSTR_LIST, FILL_JMP_TABLE)
  };
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
#ifndef CONFIG_DEBUG
    Operand ldest = { .preg = id_dest->preg };
    Operand lsrc1 = { .preg = id_src1->preg };
    Operand lsrc2 = { .preg = id_src2->preg };
#endif

    goto *(s->EHelper);

#include "all-instr.h"
    def_finish();
    IFDEF(CONFIG_DEBUG, debug_hook(s->pc, s->snpc - s->pc));
    IFDEF(CONFIG_DIFFTEST, update_gpc(lpc));
    IFDEF(CONFIG_DIFFTEST, difftest_step(s->pc, lpc));
  }
  cpu.pc = lpc;
  return n;
}
