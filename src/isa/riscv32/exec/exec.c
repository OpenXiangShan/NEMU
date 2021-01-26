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

static inline void reset_zero() { reg_l(0) = 0; }

uint32_t isa_execute(uint32_t n) {
  static const void* jmp_table[TOTAL_INSTR] = {
    MAP(INSTR_LIST, FILL_JMP_TABLE)
  };

  for (; n > 0; n --) {
    vaddr_t pc = cpu.pc;
    DecodeExecState *s = dccache_fetch(pc);
    s->npc = s->snpc;
    const void *e = s->EHelper;
    if (s->pc != pc) {
      /* Execute one instruction, including instruction fetch,
       * instruction decode, and the actual execution. */
      e = fetch_decode(s, jmp_table);
    }

    goto *e;

#include "all-instr.h"
    def_finish();

    update_pc(s);

    reset_zero();

    cpu_exec_2nd_part(pc, s->snpc, s->npc);
  }
  return n;
}
