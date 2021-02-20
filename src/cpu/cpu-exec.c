#include <cpu/cpu.h>
#include <cpu/exec.h>
#include <cpu/difftest.h>
#include <cpu/dccache.h>
#include <isa-all-instr.h>
#include <setjmp.h>

//#define PERF 1

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

static jmp_buf jbuf_exec = {};
static uint32_t n_remain;

#ifdef CONFIG_PERF_OPT
void save_globals(vaddr_t pc, uint32_t n) {
  cpu.pc = pc;
  n_remain = n;
}
#endif

void longjmp_exec(int cause) {
  longjmp(jbuf_exec, cause);
}

static int __attribute((noinline)) fetch_decode(DecodeExecState *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  IFDEF(CONFIG_DEBUG, log_bytebuf[0] = '\0');
  int idx = isa_fetch_decode(s);
  IFDEF(CONFIG_DEBUG, snprintf(s->logbuf, sizeof(s->logbuf), FMT_WORD ":   %s%*.s%s",
        s->pc, log_bytebuf, 50 - (12 + 3 * (s->snpc - s->pc)), "", log_asmbuf));
  return idx;
}

#ifdef CONFIG_PERF_OPT
#define FILL_EXEC_TABLE(name) [concat(EXEC_ID_, name)] = &&name,

#define update_lpc(npc) (lpc = (npc)) // local pc

#define rtl_j(s, target) do { update_lpc(target); s = s->next; goto finish_label; } while (0)
#define rtl_jr(s, target) do { update_lpc(*(target)); s = dccache_fetch(*(target)); goto finish_label; } while (0)
#define rtl_jrelop(s, relop, src1, src2, target) \
  do { if (interpret_relop(relop, *src1, *src2)) rtl_j(s, target); } while (0)

static uint32_t execute(uint32_t n) {
  static const void* exec_table[TOTAL_INSTR] = {
    MAP(INSTR_LIST, FILL_EXEC_TABLE)
  };
  IFDEF(PERF, static uint64_t instr, sentinel_miss, dc_miss);
  static int align_flag = 0;
  if (align_flag == 0) {
    asm volatile (".fill 48,1,0x90");
    align_flag = 1;
  }

  vaddr_t lpc = cpu.pc; // local pc
  DecodeExecState *s = dccache_fetch(lpc);
  while (true) {
    if (unlikely(s->pc != lpc)) {
      // if `s` is pointing to the sentinel, fetch the correct decode cache entry
      s = dccache_fetch(0);
      if (unlikely(s->pc != lpc)) {
        // if it is a miss in decode cache, fetch and decode the correct instruction
        s = dccache_fetch(lpc);
        cpu.pc = lpc;
        int idx = fetch_decode(s, lpc);
        s->EHelper = exec_table[idx];
        IFDEF(PERF, dc_miss ++);
      } else {
        IFDEF(PERF, sentinel_miss ++);
      }
    }

#ifdef PERF
    instr ++;
    if (instr % (65536 * 1024) == 0)
      Log("instr = %ld, sentinel_miss = %ld, dc_miss = %ld", instr, sentinel_miss, dc_miss);
#endif

    if (--n == 0) break;

    IFDEF(CONFIG_DEBUG, DecodeExecState *this_s = s);
    Operand ldest = { .preg = id_dest->preg };
    Operand lsrc1 = { .preg = id_src1->preg };
    Operand lsrc2 = { .preg = id_src2->preg };

    goto *(s->EHelper);

#undef id_dest
#undef id_src1
#undef id_src2
#define id_dest (&ldest)
#define id_src1 (&lsrc1)
#define id_src2 (&lsrc2)

#include "isa-exec.h"
    def_finish();
    IFDEF(CONFIG_DEBUG, debug_hook(this_s->pc, this_s->logbuf));
    IFDEF(CONFIG_DIFFTEST, cpu.pc = lpc);
    IFDEF(CONFIG_DIFFTEST, difftest_step(this_s->pc, lpc));
  }
  cpu.pc = lpc;
  return n;
}
#else
#include "isa-exec.h"

typedef void (*EHelper_t)(DecodeExecState *s);
#define FILL_EXEC_TABLE(name) [concat(EXEC_ID_, name)] = concat(exec_, name),

static uint32_t execute(uint32_t n) {
  static const EHelper_t exec_table[TOTAL_INSTR] = {
    MAP(INSTR_LIST, FILL_EXEC_TABLE)
  };
  DecodeExecState s;
  for (;n > 0; n --) {
    int idx = fetch_decode(&s, cpu.pc);
    cpu.pc = s.snpc;
    n_remain = n;
    exec_table[idx](&s);
    IFDEF(CONFIG_DEBUG, debug_hook(s.pc, s.logbuf));
    IFDEF(CONFIG_DIFFTEST, difftest_step(s.pc, cpu.pc));
  }
  return n;
}
#endif

/* Simulate how the CPU works. */
void cpu_exec(uint64_t nn) {
  g_print_step = (nn < MAX_INSTR_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  static uint64_t n; n = nn; // deal with setjmp()
  int ret;
  if ((ret = setjmp(jbuf_exec))) {
    uint32_t n_batch = n >= BATCH_SIZE ? BATCH_SIZE : n;
    uint32_t n_executed = n_batch - n_remain;
    n -= n_executed;
    IFUNDEF(CONFIG_DEBUG, g_nr_guest_instr += n_executed);
  }

  while (nemu_state.state == NEMU_RUNNING && n > 0) {
    uint32_t n_batch = n >= BATCH_SIZE ? BATCH_SIZE : n;
    n_remain = execute(n_batch);
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
