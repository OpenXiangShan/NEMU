#include <cpu/cpu.h>
#include <cpu/exec.h>
#include <cpu/difftest.h>
#include <isa-all-instr.h>
#include <locale.h>

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INSTR_TO_PRINT 10

CPU_state cpu = {};
uint64_t g_nr_guest_instr = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;
const rtlreg_t rzero = 0;
rtlreg_t tmp_reg[4];

void device_update();
void fetch_decode(Decode *s, vaddr_t pc);

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) log_write("%s\n", _this->logbuf);
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
#ifndef __ICS_EXPORT
  IFDEF(CONFIG_IQUEUE, iqueue_commit(_this->pc, (void *)&_this->isa.instr.val, _this->snpc - _this->pc));
  void scan_watchpoint(vaddr_t pc);
  IFDEF(CONFIG_WATCHPOINT, scan_watchpoint(_this->pc));
  IFDEF(CONFIG_DIFFTEST, save_globals(_this));
  IFDEF(CONFIG_DIFFTEST, cpu.pc = dnpc);
#endif
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
}

#ifndef CONFIG_PERF_OPT
#ifndef __ICS_EXPORT
#define rtl_priv_next(s)
#define rtl_priv_jr(s, target) rtl_jr(s, target)
#endif
#include <isa-exec.h>

#define FILL_EXEC_TABLE(name) [concat(EXEC_ID_, name)] = concat(exec_, name),
static const void* g_exec_table[TOTAL_INSTR] = {
  MAP(INSTR_LIST, FILL_EXEC_TABLE)
};

static void fetch_decode_exec_updatepc(Decode *s) {
  fetch_decode(s, cpu.pc);
  //s->EHelper(s);
  cpu.pc = s->dnpc;
}
#endif
#ifndef __ICS_EXPORT
#include <memory/host-tlb.h>

#define BATCH_SIZE 65536

IFNDEF(CONFIG_TARGET_SHARE, static uint64_t g_nr_guest_instr_end = 0);
static Decode *prev_s;

static void update_global() {
  IFDEF(CONFIG_PERF_OPT, cpu.pc = prev_s->pc);
}

void save_globals(Decode *s) {
  IFDEF(CONFIG_PERF_OPT, prev_s = s);
}

static word_t g_ex_cause = NEMU_EXEC_RUNNING;
static int g_sys_state_flag = 0;

void set_sys_state_flag(int flag) {
  g_sys_state_flag |= flag;
}

#if defined(CONFIG_ENGINE_INTERPRETER) && defined(CONFIG_MODE_SYSTEM)
void mmu_tlb_flush(vaddr_t vaddr) {
  hosttlb_flush(vaddr);
  if (vaddr == 0) set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
}
#endif

#ifndef CONFIG_TARGET_AM
#include <setjmp.h>
static jmp_buf jbuf_exec = {};

void longjmp_exec(int cause) {
  longjmp(jbuf_exec, cause);
}
#else
void longjmp_exec(int cause) {
  Assert(cause == NEMU_EXEC_END, "NEMU on AM does not support exception");
}
#endif

void longjmp_exception(int ex_cause) {
  g_ex_cause = ex_cause;
  longjmp_exec(NEMU_EXEC_EXCEPTION);
}

#ifdef CONFIG_PERF_OPT
#define FILL_EXEC_TABLE(name) [concat(EXEC_ID_, name)] = &&concat(exec_, name),

#define rtl_j(s, target) do { \
  IFDEF(CONFIG_ICOUNT_BASIC_BLOCK, n -= s->idx_in_bb); \
  IFDEF(CONFIG_ICOUNT_BASIC_BLOCK, g_nr_guest_instr += s->idx_in_bb); \
  s = s->tnext; \
  goto end_of_bb; \
} while (0)
#define rtl_jr(s, target) do { \
  IFDEF(CONFIG_ICOUNT_BASIC_BLOCK, n -= s->idx_in_bb); \
  IFDEF(CONFIG_ICOUNT_BASIC_BLOCK, g_nr_guest_instr += s->idx_in_bb); \
  s = jr_fetch(s, *(target)); \
  goto end_of_bb; \
} while (0)
#define rtl_jrelop(s, relop, src1, src2, target) do { \
  IFDEF(CONFIG_ICOUNT_BASIC_BLOCK, n -= s->idx_in_bb); \
  IFDEF(CONFIG_ICOUNT_BASIC_BLOCK, g_nr_guest_instr += s->idx_in_bb); \
  if (interpret_relop(relop, *src1, *src2)) s = s->tnext; \
  else s = s->ntnext; \
  goto end_of_bb; \
} while (0)

#define rtl_priv_next(s) do { \
  if (g_sys_state_flag) { \
    IFDEF(CONFIG_ICOUNT_PRECISE, g_nr_guest_instr ++); \
    s = (g_sys_state_flag & SYS_STATE_FLUSH_TCACHE) ? \
      tcache_handle_flush(s->snpc) : s + 1; \
    g_sys_state_flag = 0; \
    goto end_of_loop; \
  } \
} while (0)

#define rtl_priv_jr(s, target) do { \
  IFDEF(CONFIG_ICOUNT_BASIC_BLOCK, g_nr_guest_instr += s->idx_in_bb); \
  IFDEF(CONFIG_ICOUNT_PRECISE, g_nr_guest_instr ++); \
  s = jr_fetch(s, *(target)); \
  if (g_sys_state_flag & SYS_STATE_FLUSH_TCACHE) { \
    s = tcache_handle_flush(s->pc); \
    g_sys_state_flag = 0; \
  } \
  goto end_of_loop; \
} while (0)

static const void **g_exec_table;

Decode* tcache_jr_fetch(Decode *s, vaddr_t jpc);
Decode* tcache_decode(Decode *s);
void tcache_handle_exception(vaddr_t jpc);
Decode* tcache_handle_flush(vaddr_t snpc);

static Decode* jr_fetch(Decode *s, vaddr_t target) {
  if (likely(s->tnext->pc == target)) return s->tnext;
  if (likely(s->ntnext->pc == target)) return s->ntnext;
  return tcache_jr_fetch(s, target);
}

static void execute(int n) {
  static const void* local_exec_table[TOTAL_INSTR] = {
    MAP(INSTR_LIST, FILL_EXEC_TABLE)
  };
  static int init_flag = 0;
  Decode *s = prev_s;

  if (likely(init_flag == 0)) {
    g_exec_table = local_exec_table;
    extern Decode* tcache_init(const void *exec_nemu_decode, vaddr_t reset_vector);
    s = tcache_init(&&exec_nemu_decode, cpu.pc);
    IFDEF(CONFIG_MODE_SYSTEM, hosttlb_init());
    init_flag = 1;
  }

  __attribute__((unused)) Decode *this_s = s;
  while (MUXDEF(CONFIG_TARGET_AM, nemu_state.state != NEMU_RUNNING, true)) {
#if defined(CONFIG_ITRACE) || defined(CONFIG_WATCHPOINT) || \
    defined(CONFIG_IQUEUE) || defined(CONFIG_DIFFTEST)
    this_s = s;
#endif
    __attribute__((unused)) rtlreg_t ls0, ls1, ls2;

    goto *(s->EHelper);

#undef s0
#undef s1
#undef s2
#define s0 &ls0
#define s1 &ls1
#define s2 &ls2

#include <isa-exec.h>

def_EHelper(nemu_decode) {
  s = tcache_decode(s);
  continue;
}

end_of_bb:
  IFDEF(CONFIG_ICOUNT_BASIC_BLOCK, if (unlikely(n <= 0)) break);

    def_finish();
    IFDEF(CONFIG_ICOUNT_PRECISE, g_nr_guest_instr ++);
    IFDEF(CONFIG_ICOUNT_PRECISE, if (unlikely(-- n <= 0)) break);
    trace_and_difftest(this_s, s->pc);
  }

end_of_loop:
  trace_and_difftest(this_s, s->pc);
  prev_s = s;
}
#else // CONFIG_PERF_OPT
static void execute(int n) {
  static Decode s;
  prev_s = &s;
  for (;n > 0; n --) {
    fetch_decode_exec_updatepc(&s);
    IFNDEF(CONFIG_ICOUNT_DISABLE, g_nr_guest_instr ++);
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break;
  }
}
#endif // CONFIG_PERF_OPT
#endif // __ICS_EXPORT

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%ld", "%'ld")
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
#ifndef CONFIG_ICOUNT_DISABLE
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_instr);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " instr/s", g_nr_guest_instr * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
#else
  Log("Instruction count is disabled. You may enable it in menuconfig.");
#endif
}

void assert_fail_msg() {
#ifdef CONFIG_IQUEUE
  iqueue_dump();
#endif
  IFDEF(CONFIG_ENGINE_INTERPRETER, isa_reg_display());
  statistic();
}

int isa_new_fetch_decode(Decode *s);

void fetch_decode(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  int idx = isa_new_fetch_decode(s);
#ifndef CONFIG_PERF_OPT
  //s->dnpc = s->snpc;
#endif
  s->EHelper = g_exec_table[idx];
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *instr = (uint8_t *)&s->isa.instr.val;
  for (i = 0; i < ilen; i ++) {
    p += snprintf(p, 4, " %02x", instr[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.instr.val, ilen);
#endif
}

#ifndef CONFIG_TARGET_SHARE

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

#ifndef __ICS_EXPORT
  g_nr_guest_instr_end = g_nr_guest_instr + n;
  int cause = NEMU_EXEC_RUNNING;
#ifndef CONFIG_TARGET_AM
  if ((cause = setjmp(jbuf_exec))) {
    IFDEF(CONFIG_ICOUNT_BASIC_BLOCK, g_nr_guest_instr += prev_s->idx_in_bb - 1);
    update_global();
  }
#endif

  while (nemu_state.state == NEMU_RUNNING &&
      MUXDEF(CONFIG_ICOUNT_DISABLE, true, g_nr_guest_instr < g_nr_guest_instr_end)) {
    IFDEF(CONFIG_DEVICE, device_update());

    if (cause == NEMU_EXEC_EXCEPTION) {
      cause = 0;
      cpu.pc = isa_raise_intr(g_ex_cause, prev_s->pc);
      IFDEF(CONFIG_PERF_OPT, tcache_handle_exception(cpu.pc));
    } else {
#ifdef CONFIG_HAS_INTR
      word_t intr = isa_query_intr();
      if (intr != INTR_EMPTY) {
        cpu.pc = isa_raise_intr(intr, cpu.pc);
        IFDEF(CONFIG_DIFFTEST, ref_difftest_raise_intr(intr));
        IFDEF(CONFIG_PERF_OPT, tcache_handle_exception(cpu.pc));
      }
#endif
    }

    uint32_t nremain = g_nr_guest_instr_end - g_nr_guest_instr;
    int nbatch = nremain >= BATCH_SIZE ? BATCH_SIZE : nremain;
    execute(nbatch);
    update_global();
  }
#else
  Decode s;
  for (;n > 0; n --) {
    fetch_decode_exec_updatepc(&s);
    g_nr_guest_instr ++;
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
#endif

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ASNI_FMT("ABORT", ASNI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ASNI_FMT("HIT GOOD TRAP", ASNI_FG_GREEN) :
            ASNI_FMT("HIT BAD TRAP", ASNI_FG_RED))),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT: statistic();
  }
}
#else
void cpu_exec(uint64_t n) {
#ifdef CONFIG_MODE_SYSTEM
  panic("FIXME: add support of exception");
#endif

#ifdef CONFIG_PERF_OPT
  void tcache_check_and_flush(vaddr_t pc);
  tcache_check_and_flush(cpu.pc);

  if (prev_s != NULL && cpu.pc != prev_s->pc) {
    // caused by difftest_skip_ref()
    tcache_handle_exception(cpu.pc);
  }
#endif

  execute(n);
  update_global();
}
#endif
