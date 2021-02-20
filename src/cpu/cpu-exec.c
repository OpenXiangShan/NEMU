#include <cpu/exec.h>
#include <cpu/difftest.h>
#include <cpu/dccache.h>
#include <isa-all-instr.h>

//#define PERF

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INSTR_TO_PRINT 10
#define BATCH_SIZE 65536

CPU_state cpu = {};
static bool g_print_step = false;
const rtlreg_t rzero = 0;
rtlreg_t tmp_reg[4];

#ifdef CONFIG_DEBUG
static inline void debug_hook(vaddr_t pc, const char *asmbuf) {
  g_nr_guest_instr ++;

  log_write("%s\n", asmbuf);
  if (g_print_step) { puts(asmbuf); }

  void scan_watchpoint(vaddr_t pc);
  scan_watchpoint(pc);
}
#endif

static uint32_t execute(uint32_t n) {
  def_jmp_table();
#ifdef PERF
  static uint64_t instr = 0;
  static uint64_t bp_miss = 0;
  static uint64_t dc_miss = 0;
#endif

  static int align_flag = 0;
  if (align_flag == 0) {
    asm volatile (".fill 8,1,0x90");
    align_flag = 1;
  }

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
          int idx = isa_fetch_decode(s);
          s->EHelper = jmp_table[idx];
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

#include "isa-exec.h"
    def_finish();
    IFDEF(CONFIG_DEBUG, debug_hook(s->pc, s->logbuf));
    IFDEF(CONFIG_DIFFTEST, update_gpc(lpc));
    IFDEF(CONFIG_DIFFTEST, difftest_step(s->pc, lpc));
  }
  cpu.pc = lpc;
  return n;
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INSTR_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  while (nemu_state.state == NEMU_RUNNING) {
    uint32_t n_batch = n >= BATCH_SIZE ? BATCH_SIZE : n;
    uint32_t n_remain = execute(n_batch);
    uint32_t n_executed = n_batch - n_remain;
    n -= n_executed;
    IFUNDEF(CONFIG_DEBUG, g_nr_guest_instr += n_executed);

#ifdef CONFIG_DEVICE
    extern void device_update();
    device_update();
#endif

#if !defined(CONFIG_DIFFTEST) && !_SHARE
    isa_query_intr();
#endif
  }

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s\33[0m at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? "\33[1;31mABORT" :
           (nemu_state.halt_ret == 0 ? "\33[1;32mHIT GOOD TRAP" : "\33[1;31mHIT BAD TRAP")),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT:
      monitor_statistic();
  }
}
