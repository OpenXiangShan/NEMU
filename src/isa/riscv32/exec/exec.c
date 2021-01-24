#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "../local-include/rtl.h"
#include <monitor/difftest.h>
#include <monitor/monitor.h>

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

void execute(uint64_t n) {
  for (; n > 0; n --) {
    /* Execute one instruction, including instruction fetch,
     * instruction decode, and the actual execution. */
    vaddr_t this_pc = cpu.pc;
    int idx = get_idx(this_pc);
    DecodeExecState *s = &dcache[idx].s;
    s->is_jmp = 0;
    if (dcache[idx].tag == this_pc) {
      goto *dcache[idx].EHelper;
    }

    dcache[idx].tag = this_pc;
    s->seq_pc = this_pc;
    s->isa.instr.val = instr_fetch(&s->seq_pc, 4);

#include "all-instr.h"

exec_finish:
    update_pc(s);

    reset_zero();

#ifdef DEBUG
    int len = s->seq_pc - this_pc;
    void debug_hook(vaddr_t this_pc, int len);
    debug_hook(this_pc, len);
#endif

    difftest_step(this_pc, cpu.pc);

    if (nemu_state.state != NEMU_RUNNING) break;

    extern uint64_t g_nr_guest_instr;
    g_nr_guest_instr ++;
    if (g_nr_guest_instr % 65536 == 0) break; // serve device
  }
}
