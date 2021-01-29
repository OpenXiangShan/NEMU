#include <cpu/exec.h>
#include "../local-include/rtl.h"
#include <cpu/difftest.h>
#include <cpu/cpu-exec.h>
#include <cpu/dccache.h>

#define INSTR_LIST(f) \
  f(lui) f(add) f(sll) f(srl) f(slt) f(sltu) f(xor) f(or) \
  f(and) f(addi) f(slli) f(srli) f(slti) f(sltui) f(xori) f(ori) \
  f(andi) f(auipc) f(jal) f(jalr) f(beq) f(bne) f(blt) f(bge) \
  f(bltu) f(bgeu) f(lw) f(sw) f(lh) f(lb) f(lhu) f(lbu) \
  f(sh) f(sb) f(mul) f(mulh) f(mulhu) f(mulhsu) f(div) f(divu) \
  f(rem) f(remu) f(inv) f(nemu_trap) f(csrrw) f(csrrs) f(priv)

#define def_EXEC_ID(name) \
  enum { concat(EXEC_ID_, name) = __COUNTER__ }; \
  static inline int concat(table_, name) (DecodeExecState *s) { return concat(EXEC_ID_, name); }

MAP(INSTR_LIST, def_EXEC_ID)

#define INSTR_ONE(name) + 1
#define TOTAL_INSTR (0 MAP(INSTR_LIST, INSTR_ONE))

#define FILL_JMP_TABLE(name) [concat(EXEC_ID_, name)] = &&name,

#include "../local-include/decode.h"
#include "table.h"

uint32_t isa_execute(uint32_t n) {
  static const void* jmp_table[TOTAL_INSTR] = {
    MAP(INSTR_LIST, FILL_JMP_TABLE)
  };

  // initialize the pointer to the sentinel element
  DecodeExecState *s = &dccache[DCACHE_SIZE] - 1;
  while (n > 0) {
    n --;
    vaddr_t pc = cpu.pc;
    DecodeExecState *tmp = s;
    s ++; // first try sequential fetch with lowest cost
    if (unlikely(s->pc != pc)) {
      // if the last instruction is a branch, or `s` is pointing to the sentinel,
      // then try the prediction result
      s = tmp->next;
      if (unlikely(s->pc != pc)) {
        // if the prediction is wrong, re-fetch the correct decode information,
        // and update the prediction
        s = dccache_fetch(pc);
        tmp->next = s;
        if (unlikely(s->pc != pc)) {
          // if it is a miss in decode cache, fetch and decode the correct instruction
          s->pc = pc;
          fetch_decode(s, jmp_table);
        }
      }
    }

    cpu.pc = s->snpc;
    word_t rd  = id_dest->val2;
    word_t rs1 = id_src1->val2;
    word_t rs2 = id_src2->val2;

    goto *(s->EHelper);

#include "all-instr.h"
    def_finish();

    cpu_exec_2nd_part(pc, s->snpc, cpu.pc);
  }
  return n;
}
