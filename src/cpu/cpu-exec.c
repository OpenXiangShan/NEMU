/***************************************************************************************
 * Copyright (c) 2014-2021 Zihao Yu, Nanjing University
 * Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of
 *Sciences
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <cpu/cpu.h>
#include <cpu/exec.h>
#include <cpu/difftest.h>
#include <cpu/decode.h>
#include <memory/host-tlb.h>
#include <isa-all-instr.h>
#include <locale.h>
#include <setjmp.h>
#include <unistd.h>
#include <generated/autoconf.h>
#include <profiling/profiling_control.h>
#ifdef CONFIG_ISA_riscv64
#include "../isa/riscv64/local-include/intr.h"
#endif
/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INSTR_TO_PRINT 10
#ifndef CONFIG_SHARE
#define BATCH_SIZE 65536
#else
#define BATCH_SIZE 1
#endif

CPU_state cpu = {};
uint64_t g_nr_guest_instr = 0;
uint64_t g_nr_guest_instr_old = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;
const rtlreg_t rzero = 0;
rtlreg_t tmp_reg[4];

#ifdef CONFIG_DEBUG
static inline void debug_hook(vaddr_t pc, const char *asmbuf) {
  IFDEF(CONFIG_ISA_riscv64,
        Logti("nemu S-interrupt %d  mstatus: %lx", mstatus->sie, cpu.mstatus));
  Logti("%s\n", asmbuf);
  if (g_print_step) {
    puts(asmbuf);
  }

  void scan_watchpoint(vaddr_t pc);
  scan_watchpoint(pc);
}
#endif

static jmp_buf jbuf_exec = {};
static uint64_t n_remain_total;
static int n_remain;
static Decode *prev_s;

void save_globals(Decode *s) { IFDEF(CONFIG_PERF_OPT, prev_s = s); }

uint64_t get_abs_instr_count() {
#if defined(CONFIG_ENABLE_INSTR_CNT)
  int n_batch = n_remain_total >= BATCH_SIZE ? BATCH_SIZE : n_remain_total;
  uint32_t n_executed = n_batch - n_remain;
  return n_executed + g_nr_guest_instr;
#endif
  return 0;
}

static void update_instr_cnt() {
#if defined(CONFIG_ENABLE_INSTR_CNT)
  int n_batch = n_remain_total >= BATCH_SIZE ? BATCH_SIZE : n_remain_total;
  uint32_t n_executed = n_batch - n_remain;
  n_remain_total -= (n_remain_total > n_executed) ? n_executed : n_remain_total;
  IFNDEF(CONFIG_DEBUG, g_nr_guest_instr += n_executed);

  n_remain =
      n_batch > n_remain_total ? n_remain_total : n_batch; // clean n_remain
  // Loge("n_remain = %i, n_remain_total = %lu\n", n_remain, n_remain_total);
#endif
}

void monitor_statistic() {
  update_instr_cnt();
  setlocale(LC_NUMERIC, "");
  Log("host time spent = %'ld us", g_timer);
#ifdef CONFIG_ENABLE_INSTR_CNT
  Log("total guest instructions = %'ld", g_nr_guest_instr);
  if (g_timer > 0)
    Log("simulation frequency = %'ld instr/s",
        (get_abs_instr_count() + g_nr_guest_instr_old) * 1000000 / g_timer);
  else
    Log("Finish running in less than 1 us and can not calculate the simulation "
        "frequency");
#else
  Log("CONFIG_ENABLE_INSTR_CNT is not defined");
#endif
}

static word_t g_ex_cause = 0;
static int g_sys_state_flag = 0;

void set_sys_state_flag(int flag) { g_sys_state_flag |= flag; }

void mmu_tlb_flush(vaddr_t vaddr) {
  hosttlb_flush(vaddr);
  if (vaddr == 0)
    set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
}

_Noreturn void longjmp_exec(int cause) {
  Loge("Longjmp to jbuf_exec with cause: %i", cause);
  longjmp(jbuf_exec, cause);
}

_Noreturn void longjmp_exception(int ex_cause) {
#ifdef CONFIG_GUIDED_EXEC
  cpu.guided_exec = false;
#endif
  g_ex_cause = ex_cause;
#ifdef CONFIG_ISA_riscv64
  Loge("longjmp_exec(NEMU_EXEC_EXCEPTION),EX_NAME=%s",EX_NAME[ex_cause]);
#else
  Loge("longjmp_exec(NEMU_EXEC_EXCEPTION)");
#endif

  longjmp_exec(NEMU_EXEC_EXCEPTION);
}

#ifdef CONFIG_PERF_OPT
static bool manual_cpt_quit = false;
#define FILL_EXEC_TABLE(name) [concat(EXEC_ID_, name)] = &&concat(exec_, name),

#define rtl_j(s, target)                                                       \
  do {                                                                         \
    IFDEF(CONFIG_ENABLE_INSTR_CNT, n -= s->idx_in_bb);                         \
    s = s->tnext;                                                              \
    is_ctrl = true;                                                            \
    br_taken = true;                                                           \
    goto end_of_bb;                                                            \
  } while (0)
#define rtl_jr(s, target)                                                      \
  do {                                                                         \
    IFDEF(CONFIG_ENABLE_INSTR_CNT, n -= s->idx_in_bb);                         \
    s = jr_fetch(s, *(target));                                                \
    is_ctrl = true;                                                            \
    br_taken = true;                                                           \
    goto end_of_bb;                                                            \
  } while (0)
#define rtl_jrelop(s, relop, src1, src2, target)                               \
  do {                                                                         \
    IFDEF(CONFIG_ENABLE_INSTR_CNT, n -= s->idx_in_bb);                         \
    is_ctrl = true;                                                            \
    if (interpret_relop(relop, *src1, *src2)) {                                \
      s = s->tnext;                                                            \
      br_taken = true;                                                         \
    } else                                                                     \
      s = s->ntnext;                                                           \
    goto end_of_bb;                                                            \
  } while (0)

#define rtl_priv_next(s)                                                       \
  do {                                                                         \
    if (g_sys_state_flag) {                                                    \
      s = (g_sys_state_flag & SYS_STATE_FLUSH_TCACHE)                          \
              ? tcache_handle_flush(s->snpc)                                   \
              : s + 1;                                                         \
      g_sys_state_flag = 0;                                                    \
      goto end_of_loop;                                                        \
    }                                                                          \
  } while (0)

#define rtl_priv_jr(s, target)                                                 \
  do {                                                                         \
    IFDEF(CONFIG_ENABLE_INSTR_CNT, n -= s->idx_in_bb);                         \
    s = jr_fetch(s, *(target));                                                \
    if (g_sys_state_flag & SYS_STATE_FLUSH_TCACHE) {                           \
      s = tcache_handle_flush(s->pc);                                          \
      g_sys_state_flag = 0;                                                    \
    }                                                                          \
    goto end_of_loop;                                                          \
  } while (0)

static const void **g_exec_table;

Decode *tcache_jr_fetch(Decode *s, vaddr_t jpc);
Decode *tcache_decode(Decode *s);
void tcache_handle_exception(vaddr_t jpc);
Decode *tcache_handle_flush(vaddr_t snpc);

static inline Decode *jr_fetch(Decode *s, vaddr_t target) {
  if (likely(s->tnext->pc == target))
    return s->tnext;
  if (likely(s->ntnext->pc == target))
    return s->ntnext;
  return tcache_jr_fetch(s, target);
}

static inline void debug_difftest(Decode *_this, Decode *next) {
  IFDEF(CONFIG_IQUEUE, iqueue_commit(_this->pc, (void *)&_this->isa.instr.val,
                                     _this->snpc - _this->pc));
  IFDEF(CONFIG_DEBUG, debug_hook(_this->pc, _this->logbuf));
  IFDEF(CONFIG_DIFFTEST, save_globals(next));
  IFDEF(CONFIG_DIFFTEST, cpu.pc = next->pc);
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, next->pc));
}

uint64_t per_bb_profile(Decode *prev_s, Decode *s, bool control_taken) {
  uint64_t abs_inst_count = get_abs_instr_count();
  // workload_loaded set from nemu_trap
  if (profiling_state == SimpointProfiling && (workload_loaded||donot_skip_boot)) {
    simpoint_profiling(prev_s->pc, true, abs_inst_count);
    simpoint_profiling(s->pc, false, abs_inst_count);
  }

    //  if (checkpoint_taking && able_to_take &&
    //      ((recvd_manual_oneshot_cpt && !manual_cpt_quit) ||
    //      profiling_started)) {
    //    // update cpu pc to point to next pc

  //umod or not set force m mod
  extern bool able_to_take_cpt();
  bool able_to_take = able_to_take_cpt() || force_cpt_mmode;
  if (!able_to_take) {
    return abs_inst_count;
  }

  //
  if (!(workload_loaded||donot_skip_boot)) {
    return abs_inst_count;
  }

  switch (checkpoint_state) {
  case NoCheckpoint:
    return abs_inst_count;
  case ManualOneShotCheckpointing:
    if (recvd_manual_oneshot_cpt && !manual_cpt_quit) {
      break;
    } else {
      return abs_inst_count;
    }
  case ManualUniformCheckpointing:
    if (recvd_manual_uniform_cpt) {
      break;
    } else {
      return abs_inst_count;
    }
  case UniformCheckpointing:
      break;
  case SimpointCheckpointing:
      break;
  }

  cpu.pc = s->pc;

  extern bool try_take_cpt(uint64_t icount);
  bool taken = try_take_cpt(abs_inst_count);
  if (taken) {
    Log("Have taken checkpoint on pc 0x%lx", s->pc);
    if (recvd_manual_oneshot_cpt) {
      Log("Quit after taken manual cpt\n");
      nemu_state.state = NEMU_QUIT;
      manual_cpt_quit = true;
    }
  }
  return abs_inst_count;
}

static int execute(int n) {
  Logtb("Will execute %i instrs\n", n);
  static const void *local_exec_table[TOTAL_INSTR] = {
      MAP(INSTR_LIST, FILL_EXEC_TABLE)};
  static int init_flag = 0;
  Decode *s = prev_s;

  if (likely(init_flag == 0)) {
    g_exec_table = local_exec_table;
    extern Decode *tcache_init(const void *exec_nemu_decode,
                               vaddr_t reset_vector);
    s = tcache_init(&&exec_nemu_decode, cpu.pc);
    IFDEF(CONFIG_MODE_SYSTEM, hosttlb_init());
    init_flag = 1;
  }

  __attribute__((unused)) Decode *this_s = NULL;
  __attribute__((unused)) bool br_taken = false;
  __attribute__((unused)) bool is_ctrl = false;
  while (true) {
#if defined(CONFIG_DEBUG) || defined(CONFIG_DIFFTEST) || defined(CONFIG_IQUEUE)
    this_s = s;
#endif
    __attribute__((unused)) rtlreg_t ls0, ls1, ls2;
    br_taken = false;

    goto *(s->EHelper);

#undef s0
#undef s1
#undef s2
#define s0 &ls0
#define s1 &ls1
#define s2 &ls2

#include "isa-exec.h"

    def_EHelper(nemu_decode) {
      s = tcache_decode(s);
      continue;
    }

  end_of_bb:
    IFDEF(CONFIG_ENABLE_INSTR_CNT, n_remain = n);
    IFNDEF(CONFIG_ENABLE_INSTR_CNT, n--);

    // Here is per bb action
    if (is_ctrl) {
      uint64_t abs_inst_count = per_bb_profile(prev_s, s, br_taken);
      Logtb("prev pc = 0x%lx, pc = 0x%lx", prev_s->pc, s->pc);
      Logtb("Executed %ld instructions in total, pc: 0x%lx\n",
            (int64_t)abs_inst_count, prev_s->pc);
    }
    if (manual_cpt_quit) {
      Log("unlikely(manual_cpt_quit)=%ld, manual_cpt_quit=%d",
          unlikely(manual_cpt_quit), manual_cpt_quit);
    }

    if (unlikely(n <= 0))
      break;
    if (unlikely(manual_cpt_quit))
      break;

    // Here is per inst action
    // Because every instruction executed goes here, don't put Log here to
    // improve performance
    def_finish();

    // clear for recording next inst
    is_ctrl = false;
    Logti("prev pc = 0x%lx, pc = 0x%lx", prev_s->pc, s->pc);

    save_globals(s);
    debug_difftest(this_s, s);
  }

end_of_loop:
  // Here is per loop action and some priv instruction action
  Loge(
      "end_of_loop: prev pc = 0x%lx, pc = 0x%lx, total insts: %lu, remain: %lu",
      prev_s->pc, s->pc, get_abs_instr_count(), n_remain_total);
  if (is_ctrl) {
    per_bb_profile(prev_s, s, br_taken); // TODO: this should be true for mret
  }

  debug_difftest(this_s, s);
  prev_s = s;
  return n;
}
#else
#define FILL_EXEC_TABLE(name) [concat(EXEC_ID_, name)] = concat(exec_, name),

#define rtl_priv_next(s)
#define rtl_priv_jr(s, target) rtl_jr(s, target)

#include "isa-exec.h"
static const void *g_exec_table[TOTAL_INSTR] = {
    MAP(INSTR_LIST, FILL_EXEC_TABLE)};
uint64_t br_count = 0;

#ifdef CONFIG_BR_LOG
struct br_info br_log[CONFIG_BR_LOG_SIZE];
#endif // CONFIG_BR_LOG

#ifdef CONFIG_LIGHTQS

uint64_t stable_log_begin, spec_log_begin;

extern int ifetch_mmu_state;
extern int data_mmu_state;
struct lightqs_reg_ss reg_ss, spec_reg_ss;
void csr_writeback();
void csr_prepare();

void lightqs_take_reg_snapshot() {
  csr_prepare();
  reg_ss.br_cnt = br_count;
  reg_ss.inst_cnt = g_nr_guest_instr;
#ifdef CONFIG_LIGHTQS_DEBUG
  printf("current g instr cnt = %lu\n", g_nr_guest_instr);
#endif // CONFIG_LIGHTQS_DEBUG
  reg_ss.pc = cpu.pc;
  reg_ss.mstatus = cpu.mstatus;
  reg_ss.mcause = cpu.mcause;
  reg_ss.mepc = cpu.mepc;
  reg_ss.sstatus = cpu.sstatus;
  reg_ss.scause = cpu.scause;
  reg_ss.sepc = cpu.sepc;
  reg_ss.satp = cpu.satp;
  reg_ss.mip = cpu.mip;
  reg_ss.mie = cpu.mie;
  reg_ss.mscratch = cpu.mscratch;
  reg_ss.sscratch = cpu.sscratch;
  reg_ss.medeleg = cpu.medeleg;
  reg_ss.mideleg = cpu.mideleg;
  reg_ss.mtval = cpu.mtval;
  reg_ss.stval = cpu.stval;
  reg_ss.mtvec = cpu.mtvec;
  reg_ss.stvec = cpu.stvec;
  reg_ss.mode = cpu.mode;
  reg_ss.lr_addr = cpu.lr_addr;
  reg_ss.lr_valid = cpu.lr_valid;
  reg_ss.ifetch_mmu_state = ifetch_mmu_state;
  reg_ss.data_mmu_state = data_mmu_state;
#ifdef CONFIG_RVV
  reg_ss.vtype = cpu.vtype;
  reg_ss.vstart = cpu.vstart;
  reg_ss.vxsat = cpu.vxsat;
  reg_ss.vxrm = cpu.vxrm;
  reg_ss.vl = cpu.vl;
#endif // CONFIG_RVV
  for (int i = 0; i < 32; i++) {
    reg_ss.gpr[i] = cpu.gpr[i]._64;
    reg_ss.fpr[i] = cpu.fpr[i]._64;
  }
}

void lightqs_take_spec_reg_snapshot() {
  csr_prepare();
  spec_reg_ss.br_cnt = br_count;
  spec_reg_ss.inst_cnt = g_nr_guest_instr;
  spec_reg_ss.pc = cpu.pc;
  spec_reg_ss.mstatus = cpu.mstatus;
  spec_reg_ss.mcause = cpu.mcause;
  spec_reg_ss.mepc = cpu.mepc;
  spec_reg_ss.sstatus = cpu.sstatus;
  spec_reg_ss.scause = cpu.scause;
  spec_reg_ss.sepc = cpu.sepc;
  spec_reg_ss.satp = cpu.satp;
  spec_reg_ss.mip = cpu.mip;
  spec_reg_ss.mie = cpu.mie;
  spec_reg_ss.mscratch = cpu.mscratch;
  spec_reg_ss.sscratch = cpu.sscratch;
  spec_reg_ss.medeleg = cpu.medeleg;
  spec_reg_ss.mideleg = cpu.mideleg;
  spec_reg_ss.mtval = cpu.mtval;
  spec_reg_ss.stval = cpu.stval;
  spec_reg_ss.mtvec = cpu.mtvec;
  spec_reg_ss.stvec = cpu.stvec;
  spec_reg_ss.mode = cpu.mode;
  spec_reg_ss.lr_addr = cpu.lr_addr;
  spec_reg_ss.lr_valid = cpu.lr_valid;
  spec_reg_ss.ifetch_mmu_state = ifetch_mmu_state;
  spec_reg_ss.data_mmu_state = data_mmu_state;
#ifdef CONFIG_RVV
  spec_reg_ss.vtype = cpu.vtype;
  spec_reg_ss.vstart = cpu.vstart;
  spec_reg_ss.vxsat = cpu.vxsat;
  spec_reg_ss.vxrm = cpu.vxrm;
  spec_reg_ss.vl = cpu.vl;
#endif // CONFIG_RVV
  for (int i = 0; i < 32; i++) {
    spec_reg_ss.gpr[i] = cpu.gpr[i]._64;
    spec_reg_ss.fpr[i] = cpu.fpr[i]._64;
  }
}

uint64_t lightqs_restore_reg_snapshot(uint64_t n) {
#ifdef CONFIG_LIGHTQS_DEBUG
  printf("lightqs restore reg n = %lu\n", n);
  printf("lightqs origin reg_ss inst cnt %lu\n", reg_ss.br_cnt);
#endif // CONFIG_LIGHTQS_DEBUG
  if (spec_log_begin <= n) {
#ifdef CONFIG_LIGHTQS_DEBUG
    printf("lightqs using spec snapshot\n");
#endif // CONFIG_LIGHTQS_DEBUG
    memcpy(&reg_ss, &spec_reg_ss, sizeof(reg_ss));
  }
  br_count = reg_ss.br_cnt;
  g_nr_guest_instr = reg_ss.inst_cnt;
  cpu.pc = reg_ss.pc;
  cpu.mstatus = reg_ss.mstatus;
  cpu.mcause = reg_ss.mcause;
  cpu.mepc = reg_ss.mepc;
  cpu.sstatus = reg_ss.sstatus;
  cpu.scause = reg_ss.scause;
  cpu.sepc = reg_ss.sepc;
  cpu.satp = reg_ss.satp;
  cpu.mip = reg_ss.mip;
  cpu.mie = reg_ss.mie;
  cpu.mscratch = reg_ss.mscratch;
  cpu.sscratch = reg_ss.sscratch;
  cpu.medeleg = reg_ss.medeleg;
  cpu.mideleg = reg_ss.mideleg;
  cpu.mtval = reg_ss.mtval;
  cpu.stval = reg_ss.stval;
  cpu.mode = reg_ss.mode;
  cpu.lr_addr = reg_ss.lr_addr;
  cpu.lr_valid = reg_ss.lr_valid;
  ifetch_mmu_state = reg_ss.ifetch_mmu_state;
  data_mmu_state = reg_ss.data_mmu_state;
#ifdef CONFIG_RVV
  cpu.vtype = reg_ss.vtype;
  cpu.vstart = reg_ss.vstart;
  cpu.vxsat = reg_ss.vxsat;
  cpu.vxrm = reg_ss.vxrm;
  cpu.vl = reg_ss.vl;
#endif // CONFIG_RVV
  for (int i = 0; i < 32; i++) {
    cpu.gpr[i]._64 = reg_ss.gpr[i];
    cpu.fpr[i]._64 = reg_ss.fpr[i];
  }
  csr_writeback();
#ifdef CONFIG_LIGHTQS_DEBUG
  printf("lightqs restore inst_cnt %lu\n", reg_ss.inst_cnt);
#endif // CONFIG_LIGHTQS_DEBUG
  return n - reg_ss.inst_cnt;
}

#endif // CONFIG_LIGHTQS

static int execute(int n) {
  static Decode s;
  prev_s = &s;
  for (; n > 0; n--) {
#ifdef CONFIG_LIGHTQS_DEBUG
    printf("ahead pc %lx %lx\n", g_nr_guest_instr, cpu.pc);
#endif // CONFIG_LIGHTQS_DEBUG
    cpu.amo = false;
    fetch_decode(&s, cpu.pc);
    cpu.debug.current_pc = s.pc;
    cpu.pc = s.snpc;
#ifdef CONFIG_TVAL_EX_II
    cpu.instr = s.isa.instr.val;
#endif
#ifdef CONFIG_SHARE
    if (unlikely(dynamic_config.debug_difftest)) {
      fprintf(stderr, "(%d) [NEMU] pc = 0x%lx inst %x\n", getpid(), s.pc,
              s.isa.instr.val);
    }
#endif
    s.EHelper(&s);
    g_nr_guest_instr++;
#ifdef CONFIG_BR_LOG
#ifdef CONFIG_LIGHTQS_DEBUG
    if (g_nr_guest_instr == 10000) {
      // print out to file
      // FILE *f = fopen("/nfs/home/chenguokai/NEMU_ahead/ahead.txt", "w");
      for (int i = 0; i < 2000; i++) {
        fprintf(stdout, "%010lx %d %d %010lx\n", br_log[i].pc, br_log[i].taken,
                br_log[i].type, br_log[i].target);
      }
      // fclose(f);
    }
#endif // CONFIG_LIGHTQS_DEBUG
#endif // CONFIG_BR_LOG
    IFDEF(CONFIG_DEBUG, debug_hook(s.pc, s.logbuf));
    IFDEF(CONFIG_DIFFTEST, difftest_step(s.pc, cpu.pc));
    if (isa_query_intr() != INTR_EMPTY) {
      break;
    }
    if (nemu_state.state == NEMU_STOP) {
      break;
    }
  }
  return n;
}
#endif

void fetch_decode(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  IFDEF(CONFIG_DEBUG, log_bytebuf[0] = '\0');
  int idx = isa_fetch_decode(s);
  Logtid(FMT_WORD ":   %s%*.s%s", s->pc, log_bytebuf,
         40 - (12 + 3 * (int)(s->snpc - s->pc)), "", log_asmbuf);
  IFDEF(CONFIG_DEBUG,
        snprintf(s->logbuf, sizeof(s->logbuf), FMT_WORD ":   %s%*.s%s", s->pc,
                 log_bytebuf, 40 - (12 + 3 * (int)(s->snpc - s->pc)), "",
                 log_asmbuf));
  s->EHelper = g_exec_table[idx];
}

#ifdef CONFIG_PERF_OPT
static void update_global() {
  update_instr_cnt();
  cpu.pc = prev_s->pc;
}
#endif

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
#ifndef CONFIG_LIGHTQS
  IFDEF(CONFIG_SHARE, assert(n <= 1));
#endif
  g_print_step = (n < MAX_INSTR_TO_PRINT);
  switch (nemu_state.state) {
  case NEMU_END:
  case NEMU_ABORT:
    printf("Program execution has ended. To restart the program, exit NEMU and "
           "run again.\n");
#ifdef CONFIG_BR_LOG
    printf("debug: bridx = %ld\n", br_count);
#endif // CONFIG_BR_LOG
    return;
  default:
    nemu_state.state = NEMU_RUNNING;
    Loge("Setting NEMU state to RUNNING");
  }

  uint64_t timer_start = get_time();

  n_remain_total = n; // + AHEAD_LENGTH; // deal with setjmp()
  Loge("cpu_exec will exec %lu instrunctions", n_remain_total);
  int cause;
  if ((cause = setjmp(jbuf_exec))) {
    n_remain -= prev_s->idx_in_bb - 1;
    // Here is exception handle
#ifdef CONFIG_PERF_OPT
    update_global();
#endif
    Loge("After update_global, n_remain: %i, n_remain_total: %li", n_remain,
         n_remain_total);
  }

  while (nemu_state.state == NEMU_RUNNING &&
         MUXDEF(CONFIG_ENABLE_INSTR_CNT, n_remain_total > 0, true)) {
#ifdef CONFIG_DEVICE
    extern void device_update();
    device_update();
#endif

#ifndef CONFIG_SHARE
#ifdef LIGHTQS
    extern void pmem_record_reset();
    extern void clint_take_snapshot();
    pmem_record_reset();
    lightqs_take_reg_snapshot();
    clint_take_snapshot();
#endif // LIGHTQS
#endif // CONFIG_SHARE

    if (cause == NEMU_EXEC_EXCEPTION) {
      Loge("Handle NEMU_EXEC_EXCEPTION");
      cause = 0;
      cpu.pc = raise_intr(g_ex_cause, prev_s->pc);
      cpu.amo = false; // clean up
      IFDEF(CONFIG_PERF_OPT, tcache_handle_exception(cpu.pc));
      IFDEF(CONFIG_SHARE, break);
    } else {
      word_t intr = MUXDEF(CONFIG_SHARE, INTR_EMPTY, isa_query_intr());
      if (intr != INTR_EMPTY) {
        Loge("NEMU raise intr");
        cpu.pc = raise_intr(intr, cpu.pc);
        IFDEF(CONFIG_DIFFTEST, ref_difftest_raise_intr(intr));
        IFDEF(CONFIG_PERF_OPT, tcache_handle_exception(cpu.pc));
      }
    }

    int n_batch = n_remain_total >= BATCH_SIZE ? BATCH_SIZE : n_remain_total;
    n_remain = execute(n_batch);
#ifdef CONFIG_PERF_OPT
    // return from execute
    update_global(cpu.pc);
    Loge("n_remain_total: %lu", n_remain_total);
#else
    n_remain_total -= n_batch;

#endif
  }

#ifndef CONFIG_SHARE
#ifdef CONFIG_LIGHTQS
  // restore to expected point
  void pmem_record_restore(uint64_t restore_inst_cnt);
  pmem_record_restore(reg_ss.inst_cnt);
  uint64_t remain_inst_cnt = lightqs_restore_reg_snapshot(n);
  extern void clint_restore_snapshot();
  clint_restore_snapshot();
  execute(remain_inst_cnt);

  extern void dump_pmem();
  extern void dump_regs();
  dump_pmem();
  dump_regs();
#endif // CONFIG_LIGHTQS
#endif // CONFIG_SHARE

  // If nemu_state.state is NEMU_RUNNING, n_remain_total should be zero.
  if (nemu_state.state == NEMU_RUNNING) {
    nemu_state.state = NEMU_QUIT;
  }

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
  case NEMU_RUNNING:
    nemu_state.state = NEMU_STOP;
    Loge("NEMU stopped when running");
    if (ISDEF(CONFIG_EXITLOG)) {
      monitor_statistic();
    }
    break;

  case NEMU_END:
  case NEMU_ABORT:
    Log("nemu: %s\33[0m at pc = " FMT_WORD,
        (nemu_state.state == NEMU_ABORT
             ? "\33[1;31mABORT"
             : (nemu_state.halt_ret == 0 ? "\33[1;32mHIT GOOD TRAP"
                                         : "\33[1;31mHIT BAD TRAP")),
        nemu_state.halt_pc);
    Log("trap code:%d ,abs guest_instr = %ld ",
        nemu_state.halt_ret , get_abs_instr_count());
    monitor_statistic();
    break;
  case NEMU_QUIT:
#ifndef CONFIG_SHARE
    monitor_statistic();
    extern char *mapped_cpt_file; // defined in paddr.c
    if (mapped_cpt_file != NULL) {
      extern void serialize_reg_to_mem();
      serialize_reg_to_mem();
    }
#else
    break;
#endif
  }
}
