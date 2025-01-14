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
#include "../local-include/trigger.h"
#include "../local-include/aia.h"
#include "macro.h"

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

// Global number of guest instructions
uint64_t g_nr_guest_instr = 0;
// Note: Always use get_abs_instr_count() to get the instr count
// In BY_BB mode, g_nr_guest_instr is updated in update_instr_cnt().
// In BY_INSTR mode, g_nr_guest_instr is updated in execute().

uint64_t g_nr_vst = 0, g_nr_vst_unit = 0, g_nr_vst_unit_optimized = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;
const rtlreg_t rzero = 0;
rtlreg_t tmp_reg[4];

static uint64_t n_remain_total; // instructions remaining in cpu_exec()
static int n_remain;            // instructions remaining in execute()
static int n_batch;             // instructions that execute() plans to batch
// Note: In BY_BB mode, n_remain and n_batch are used in instuction counting calculating.
// Note: n_remain is setup and mainly updated in execute(), but it is also updated after longjmp from execute().
// Note: n_remain and n_batch is cleared in update_instr_cnt() to avoid duplicate updating of g_nr_guest_instr.
#ifdef CONFIG_INSTR_CNT_BY_BB
  static int instr_count_bb_unsettled = 0;  // unsettled number of instruction when BATCH ends but BB does not end.
#endif

// Note for BY_BB mode:
// - g_nr_guest_instr provides BATCH-level instr-count.
// - n_remain and n_batch provide BB-level instr-count.
// - s->idx_in_bb provides INSTR-level instr-count.
// - instr_count_bb_unsettled handles special case where BATCH is end but BB is not end.

Decode *prev_s;

#ifdef CONFIG_DEBUG
static inline void debug_hook(vaddr_t pc, const char *asmbuf) {
  Logti("%s", asmbuf);
  if (g_print_step) {
    puts(asmbuf);
  }

  void scan_watchpoint(vaddr_t pc);
  scan_watchpoint(pc);
}
#endif


void save_globals(Decode *s) { IFDEF(CONFIG_PERF_OPT, prev_s = s); }

// Get the number of executed instructions:
// Two function is provided: get_abs_instr_count() and get_abs_instr_count_csr()
// For INSTR-level (INSTR_CNT_BY_INSTR), there is no different between two function.
// For BB-level (INSTR_CNT_BY_BB):
//  - get_abs_instr_count() returns bb-level instruction counts, and can be used everywhere.
//  - get_abs_instr_count_csr() returns instr-level instruction counts, and should only be used during instrcution execution.

// The integration of these two functions is challenging due to the following issue:
//   After one basic block ends, idx_in_bb is settled into n_remain, but s->idx_in_bb could not be clear.
//   As a result, get_abs_instr_count_csr() will return an incorrect value with a duplicate size of bb.

// This function returns the number of executed instructions. The accuracy depens on config.
// This is widely used for profiling, timer and so on.
// Refer to the comment above for more technical details.
uint64_t get_abs_instr_count() {
#if defined(CONFIG_INSTR_CNT_BY_BB)
  // BB-level
  uint32_t n_executed = n_batch - n_remain;
  return n_executed + g_nr_guest_instr;
#elif defined(CONFIG_INSTR_CNT_BY_INSTR)
  // INSTR-level
  return g_nr_guest_instr;
#else // CONFIG_INSTR_CNT_DISABLED
  return 0;
#endif
}

// This function returns the number of executed instructions. The accuracy is always instruction. 
// This is only used in CSR read & write, for basic counters (mcycle, minstret, cycle, instret)
// Refer to the comment above for more technical details.
uint64_t get_abs_instr_count_csr() {
#if defined(CONFIG_INSTR_CNT_BY_BB)
  return get_abs_instr_count() + prev_s->idx_in_bb - 1;
  // s of this instruction is saved into prev_s, before csrrw() in rtl_sys_slow_path().
#elif defined(CONFIG_INSTR_CNT_BY_INSTR)
  // INSTR-level
  return g_nr_guest_instr;
#else // CONFIG_INSTR_CNT_DISABLED
  return 0;
#endif
}

static void update_instr_cnt() {
#ifdef CONFIG_ENABLE_INSTR_CNT
  uint32_t n_executed = n_batch - n_remain;

  // clear n_batch and n_remain to avoid duplicate updates.
  n_batch = 0;
  n_remain = 0;

  if (n_remain_total != -1) {
    // update n_remain_total, While preventing n_remain_total from becoming negative.
    n_remain_total -= MIN_OF(n_remain_total, n_executed);
  }

  // update g_nr_guest_instr, only if instr count is by bb.
  IFDEF(CONFIG_INSTR_CNT_BY_BB, g_nr_guest_instr += n_executed);
#endif // CONFIG_ENABLE_INSTR_CNT
}

void monitor_statistic() {
  setlocale(LC_NUMERIC, "");
  Log("host time spent = %'ld us", g_timer);
#ifdef CONFIG_ENABLE_INSTR_CNT
  Log("total guest instructions = %'ld", g_nr_guest_instr);
  Log("vst count = %'ld, vst unit count = %'ld, vst unit optimized count = %'ld",
      g_nr_vst, g_nr_vst_unit, g_nr_vst_unit_optimized);
  if (g_timer > 0)
    Log("simulation frequency = %'ld instr/s",
        g_nr_guest_instr * 1000000 / g_timer);
  else
    Log("Finish running in less than 1 us and can not calculate the simulation "
        "frequency");
#else
  Log("Instruction counting is disabled.");
#endif
  fflush(stdout);
}

static word_t g_ex_cause = 0;
static int g_sys_state_flag = 0;

void set_sys_state_flag(int flag) { g_sys_state_flag |= flag; }

void mmu_tlb_flush(vaddr_t vaddr) {
  hosttlb_flush(vaddr);
  if (vaddr == 0)
    set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
}

jmp_buf context_stack[CONTEXT_STACK_SIZE] = {};
int context_idx = -1;

void pop_context() {
  if (context_idx < 0) {
    panic("Unexcepted exeception context idx = %d", context_idx);
  }
  context_idx--;
}

_Noreturn void longjmp_context(int cause) {
  Loge("Longjmp to jbuf_exec with cause: %i", cause);
  if (context_idx < 0) {
    panic("Unexcepted exeception context idx = %d", context_idx);
  }
  longjmp(context_stack[context_idx], cause);
}

_Noreturn void longjmp_exception(int ex_cause) {
  if (context_idx == 0) {
    // context_idx == 0 means only the execute loop context saved.
  #ifdef CONFIG_GUIDED_EXEC
    cpu.guided_exec = false;
  #endif
    g_ex_cause = ex_cause;
    Loge("longjmp_context(NEMU_EXEC_EXCEPTION)");
    longjmp_context(NEMU_EXEC_EXCEPTION);
  } else {
    // For other condition, we should pass parameter ex_cause to longjmp
    longjmp_context(ex_cause);
  }
}

#ifdef CONFIG_PERF_OPT
static bool manual_cpt_quit = false;
#define FILL_EXEC_TABLE(name) [concat(EXEC_ID_, name)] = &&concat(exec_, name),

// this rtl_j() is only used in PERF_OPT
#define rtl_j(s, target)                                                       \
  do {                                                                         \
    /* Settle instruction counting for the last bb. */                         \
    IFDEF(CONFIG_INSTR_CNT_BY_BB, n_remain -= s->idx_in_bb);                   \
    s = s->tnext;                                                              \
    is_ctrl = true;                                                            \
    br_taken = true;                                                           \
    goto end_of_bb;                                                            \
  } while (0)

// this rtl_jr() is only used in PERF_OPT
#define rtl_jr(s, target)                                                      \
  do {                                                                         \
    /* Settle instruction counting for the last bb. */                         \
    IFDEF(CONFIG_INSTR_CNT_BY_BB, n_remain -= s->idx_in_bb);                   \
    s = jr_fetch(s, *(target));                                                \
    is_ctrl = true;                                                            \
    br_taken = true;                                                           \
    goto end_of_bb;                                                            \
  } while (0)

// this rtl_jrelop() is only used in PERF_OPT
#define rtl_jrelop(s, relop, src1, src2, target)                               \
  do {                                                                         \
    /* Settle instruction counting for the last bb. */                         \
    IFDEF(CONFIG_INSTR_CNT_BY_BB, n_remain -= s->idx_in_bb);                   \
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
      if (g_sys_state_flag & SYS_STATE_FLUSH_TCACHE) {                         \
        /* Settle instruction counting for the unended bb. */                  \
        IFDEF(CONFIG_INSTR_CNT_BY_BB, n_remain -= s->idx_in_bb);               \
        s = tcache_handle_flush(s->snpc);                                      \
      } else {                                                                 \
        /* BB has not end, but BATCH is about to end. */                       \
        /* Record unsettled instruction number for feature flush. */           \
        IFDEF(CONFIG_INSTR_CNT_BY_BB, instr_count_bb_unsettled = s->idx_in_bb);\
        s = s + 1;                                                             \
      }                                                                        \
      g_sys_state_flag = 0;                                                    \
      goto end_of_loop;                                                        \
    }                                                                          \
  } while (0)

#define rtl_priv_jr(s, target)                                                 \
  do {                                                                         \
    /* Settle instruction counting for the last bb. */                         \
    IFDEF(CONFIG_INSTR_CNT_BY_BB, n_remain -= s->idx_in_bb);                   \
    is_ctrl = true;                                                            \
    s = jr_fetch(s, *(target));                                                \
    if (g_sys_state_flag) {                                                    \
      if (g_sys_state_flag & SYS_STATE_FLUSH_TCACHE) {                         \
        s = tcache_handle_flush(s->pc);                                        \
      }                                                                        \
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

#ifndef CONFIG_SHARE
uint64_t per_bb_profile(Decode *prev_s, Decode *s, bool control_taken) {
  // checkpoint_icount_base is set from nemu_trap.
  // Profiling and checkpointing use this as the starting point for instruction counting.
  uint64_t abs_inst_count = get_abs_instr_count() - checkpoint_icount_base;
  // workload_loaded set from nemu_trap
  if (profiling_state == SimpointProfiling && (workload_loaded||donot_skip_boot)) {
    simpoint_profiling(prev_s->pc, true, abs_inst_count);
    simpoint_profiling(s->pc, false, abs_inst_count);
  }

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
#endif // CONFIG_SHARE

static void execute(int n) {
  Logtb("execute() Will execute %i instrs\n", n);
  n_remain = n;
  // Note: n_remain in PERF_OPT execute may be less than 0, as it is only computed at end of basic block.
  // Note: n is no longer used below, use n_remain instead.
  // NEMU could longjmp out of execute(), so return value makes no sense.
  IFDEF(CONFIG_INSTR_CNT_BY_BB, instr_count_bb_unsettled = 0);

  static const void *local_exec_table[TOTAL_INSTR] = {
      MAP(INSTR_LIST, FILL_EXEC_TABLE)
  };
  static int init_flag = 0;
  Decode *s = prev_s;

  if (likely(init_flag == 0)) {
    g_exec_table = local_exec_table;
    extern Decode *tcache_init(const void *exec_nemu_decode, vaddr_t reset_vector);
    s = tcache_init(&&exec_nemu_decode, cpu.pc);
    IFDEF(CONFIG_MODE_SYSTEM, hosttlb_init());
    init_flag = 1;
  }

  __attribute__((unused)) Decode *this_s = NULL;
  __attribute__((unused)) bool br_taken = false;
  __attribute__((unused)) bool is_ctrl = false;

  // main loop
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
    // Here is per bb action.
    // When jump or branch happens, it will goto here.

    // Exit the execute() loop after certain basic blocks, even if instr count is disabled.
    IFDEF(CONFIG_INSTR_CNT_DISABLED, n_remain -= 1);

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

    if (unlikely(n_remain <= 0)) {
      // goto end_of_loop;
      break;
    }
    if (unlikely(manual_cpt_quit)) {
      // goto end_of_loop;
      break;
    }

    def_finish();
    // Here is per inst action.
    // Most of executed instruction (except some priv instructions) will goto here.
    // Don't put Log here to improve performance

    // clear for recording next inst
    is_ctrl = false;
    Logti("prev pc = 0x%lx, pc = 0x%lx", prev_s->pc, s->pc);

    IFDEF(CONFIG_INSTR_CNT_BY_INSTR, g_nr_guest_instr += 1);
    IFDEF(CONFIG_INSTR_CNT_BY_INSTR, n_remain -= 1);

    save_globals(s);
    debug_difftest(this_s, s);
  }

end_of_loop:
  // Here is per loop action and some priv instruction action.
  // If end_of_bb and n_remain <= 0, it will goto here.
  // Most of priv instruction (using rtl_sys_slow_path()) will goto here.

  // Settle instruction counting for the last instruction:
  // - If it is end_of_bb and n_remain < 0, it will goto here without "per inst action".
  // - If it is priv instruction, it will goto here without "per inst action".
  IFDEF(CONFIG_INSTR_CNT_BY_INSTR, g_nr_guest_instr += 1);
  IFDEF(CONFIG_INSTR_CNT_BY_INSTR, n_remain -= 1);
  // g_nr_guest_instr_temp += 1;

  Logti("end_of_loop: prev pc = 0x%lx, pc = 0x%lx", prev_s->pc, s->pc);
  Loge("total insts: %'lu, execute remain: %'d", get_abs_instr_count(), n_remain);

  if (is_ctrl) {
    per_bb_profile(prev_s, s, br_taken);
  }

  if (manual_cpt_quit) {
    Log("unlikely(manual_cpt_quit)=%ld, manual_cpt_quit=%d",
        unlikely(manual_cpt_quit), manual_cpt_quit);
  }

  debug_difftest(this_s, s);
  save_globals(s);
}
#else
#define FILL_EXEC_TABLE(name) [concat(EXEC_ID_, name)] = concat(exec_, name),

#define rtl_priv_next(s)
#define rtl_priv_jr(s, target) rtl_jr(s, target)

#include "isa-exec.h"
static const void *g_exec_table[TOTAL_INSTR] = {
    MAP(INSTR_LIST, FILL_EXEC_TABLE)};

#ifdef CONFIG_LIGHTQS

uint64_t stable_log_begin, spec_log_begin;

extern int ifetch_mmu_state;
extern int data_mmu_state;
struct lightqs_reg_ss reg_ss, spec_reg_ss;
void csr_writeback();
void csr_prepare();

void lightqs_take_reg_snapshot() {
  csr_prepare();
  reg_ss.br_cnt = br_log_get_count();
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
  #ifdef CONFIG_RV_MBMC
  reg_ss.mbmc = cpu.mbmc;
  #endif
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
  spec_reg_ss.br_cnt = br_log_get_count();
  spec_reg_ss.inst_cnt = g_nr_guest_instr;
  spec_reg_ss.pc = cpu.pc;
  spec_reg_ss.mstatus = cpu.mstatus;
  spec_reg_ss.mcause = cpu.mcause;
  spec_reg_ss.mepc = cpu.mepc;
  spec_reg_ss.sstatus = cpu.sstatus;
  spec_reg_ss.scause = cpu.scause;
  spec_reg_ss.sepc = cpu.sepc;
  spec_reg_ss.satp = cpu.satp;
  #ifdef CONFIG_RV_MBMC
  spec_reg_ss.mbmc = cpu.mbmc;
  #endif
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
  br_log_set_count(reg_ss.br_cnt);
  g_nr_guest_instr = reg_ss.inst_cnt;
  cpu.pc = reg_ss.pc;
  cpu.mstatus = reg_ss.mstatus;
  cpu.mcause = reg_ss.mcause;
  cpu.mepc = reg_ss.mepc;
  cpu.sstatus = reg_ss.sstatus;
  cpu.scause = reg_ss.scause;
  cpu.sepc = reg_ss.sepc;
  cpu.satp = reg_ss.satp;
  #ifdef CONFIG_RV_MBMC
  cpu.mbmc = reg_ss.mbmc;
  #endif
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

static void execute(int n) {
  static Decode s;
  prev_s = &s;
  for (n_remain = n; n_remain > 0; n_remain -= 1) {
#ifdef CONFIG_LIGHTQS_DEBUG
    printf("ahead pc %lx %lx\n", g_nr_guest_instr, cpu.pc);
#endif // CONFIG_LIGHTQS_DEBUG
    cpu.amo = false;
    fetch_decode(&s, cpu.pc);
    cpu.debug.current_pc = s.pc;
    cpu.pc = s.snpc;
#ifdef CONFIG_SHARE
    if (unlikely(dynamic_config.debug_difftest)) {
      fprintf(stderr, "(%d) [NEMU] pc = 0x%lx inst %x\n", getpid(), s.pc,
              s.isa.instr.val);
    }
#endif
    s.EHelper(&s);

    IFDEF(CONFIG_INSTR_CNT_BY_INSTR, g_nr_guest_instr += 1);

    IFDEF(CONFIG_IQUEUE, iqueue_commit(s.pc, (void *)&s.isa.instr.val, s.snpc - s.pc));
    IFDEF(CONFIG_DEBUG, debug_hook(s.pc, s.logbuf));
    IFDEF(CONFIG_DIFFTEST, difftest_step(s.pc, cpu.pc));

    #ifdef CONFIG_ISA_riscv64
      #ifdef CONFIG_DETERMINISTIC
        void update_riscv_timer();
        update_riscv_timer();
      #endif // CONFIG_DETERMINISTIC
    #endif // CONFIG_ISA_riscv64

    if (MUXDEF(CONFIG_SHARE, INTR_EMPTY, isa_query_intr()) != INTR_EMPTY) {
      n_remain -= 1; // manually do this, as it will be skipped after break.
      break;
    }
    if (nemu_state.state == NEMU_STOP) {
      n_remain -= 1; // manually do this, as it will be skipped after break.
      break;
    }
  }
  Loge("total insts: %'lu, execute remain: %'d", get_abs_instr_count(), n_remain);
}
#endif

IFDEF(CONFIG_DEBUG, char log_bytebuf[80] = {};)
// max size is (strlen(str(instr)) + strlen(suffix_char(id_dest->width)) + sizeof(id_dest->str) + sizeof(id_src2->str) + sizeof(id_src1->str))
IFDEF(CONFIG_DEBUG, char log_asmbuf[80 + (sizeof(((Operand*)0)->str) * 3)] = {};)

void fetch_decode(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  IFDEF(CONFIG_DEBUG, log_bytebuf[0] = '\0');
  int idx = isa_fetch_decode(s);
  Logtid(FMT_WORD ":   %s%*.s%s", s->pc, MUXDEF(CONFIG_DEBUG, log_bytebuf, ""),
         40 - (12 + 3 * (int)(s->snpc - s->pc)), "", MUXDEF(CONFIG_DEBUG, log_asmbuf, ""));
  IFDEF(CONFIG_DEBUG,
        snprintf(s->logbuf, sizeof(s->logbuf), FMT_WORD ":   %s%*.s%s", s->pc,
                 log_bytebuf, 40 - (12 + 3 * (int)(s->snpc - s->pc)), "",
                 log_asmbuf));
  s->EHelper = g_exec_table[idx];
}

#ifdef CONFIG_PERF_OPT
static void update_global() {
  cpu.pc = prev_s->pc;
}
#endif

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  #ifndef CONFIG_LIGHTQS
    IFDEF(CONFIG_SHARE, assert(n <= 1));
  #endif
  g_print_step = ISNDEF(CONFIG_SHARE) && (n < MAX_INSTR_TO_PRINT);
  switch (nemu_state.state) {
  case NEMU_END:
  case NEMU_ABORT:
    printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
    return;
  default:
    nemu_state.state = NEMU_RUNNING;
    Loge("Setting NEMU state to RUNNING");
  }

  uint64_t timer_start = get_time();

  n_remain_total = n; // + AHEAD_LENGTH; // deal with setjmp()
  Loge("cpu_exec will exec %lu instrunctions", n_remain_total);

  // Prepare for longjmp
  int cause;
  PUSH_CONTEXT(&cause);
  if (cause) {
    // longjmp happened. Search longjmp_context for details.
    // Here is exception handle.

    // Settle instruction counting for the last BB.
    // When longjmp happened, there is no chance to do instruction counting for bb.
    IFDEF(CONFIG_INSTR_CNT_BY_BB, n_remain -= prev_s->idx_in_bb - 1);
    // No need to update n_remain or g_nr_guest_instr for INSTR_CNT_BY_INSTR:
    // This instruction has not finished executing, while the last instruction has been counted.

    // Record instruction for exception.
    IFDEF(CONFIG_TVAL_EX_II, cpu.instr = prev_s->isa.instr.val);

    // settle instruction counting, as BATCH has ended.
    update_instr_cnt();

    IFDEF(CONFIG_PERF_OPT, update_global());

    Loge("Longjmp happened. total insts: %'lu, cpu_exec remain: %'li", get_abs_instr_count(), n_remain_total);
  }

  while (nemu_state.state == NEMU_RUNNING &&
         MUXDEF(CONFIG_ENABLE_INSTR_CNT, n_remain_total > 0, true)) {
    #ifdef CONFIG_DEVICE
      extern void device_update();
      device_update();
    #endif

    #ifdef CONFIG_ISA_riscv64
      void update_riscv_timer();
      update_riscv_timer();
    #endif // CONFIG_ISA_riscv64

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
      IFDEF(CONFIG_TDATA1_ETRIGGER, trig_action_t action = check_triggers_etrigger(cpu.TM, g_ex_cause));

      cpu.pc = raise_intr(g_ex_cause, prev_s->pc);
      cpu.amo = false; // clean up

      // No need to settle instruction counting here, as it is done in longjmp handler.
      // It's necessary to flush tcache for exception: addr space may conflict in different priv/mmu mode.
      IFDEF(CONFIG_PERF_OPT, tcache_handle_flush(cpu.pc));

      // Trigger may raise EX_BP and perform longjmp.
      IFDEF(CONFIG_TDATA1_ETRIGGER, trigger_handler(TRIG_TYPE_ETRIG, action, 0));

      // End main loop as difftest ref.
      IFDEF(CONFIG_SHARE, break);

    } else {
      // Check interrupt
      word_t intr = MUXDEF(CONFIG_SHARE, INTR_EMPTY, isa_query_intr());
      if (intr != INTR_EMPTY) {
        Loge("NEMU raise intr");
        #ifdef CONFIG_TDATA1_ICOUNT
          trig_action_t icount_action = check_triggers_icount(cpu.TM);
          trigger_handler(TRIG_TYPE_ICOUNT, icount_action, 0);
        #endif // CONFIG_TDATA1_ICOUNT
        IFDEF(CONFIG_TDATA1_ITRIGGER, trig_action_t itrigger_action = check_triggers_itrigger(cpu.TM, intr));

        cpu.pc = raise_intr(intr, cpu.pc);

        // Settle instruction counting for not end BB.
        #ifdef CONFIG_INSTR_CNT_BY_BB
          g_nr_guest_instr += instr_count_bb_unsettled;
          instr_count_bb_unsettled = 0;
        #endif
        // No need to update_instr_count(). This is not the end of BATCH.

        // It's necessary to flush tcache for interrupt: addr space may conflict in different priv/mmu mode.
        IFDEF(CONFIG_PERF_OPT, tcache_handle_flush(cpu.pc));

        IFDEF(CONFIG_TDATA1_ITRIGGER, trigger_handler(TRIG_TYPE_ITRIG, itrigger_action, 0));
        IFDEF(CONFIG_DIFFTEST, ref_difftest_raise_intr(intr));
      }
    }

    n_batch = MIN_OF(n_remain_total, BATCH_SIZE);
    execute(n_batch);

    // settle instruction counting, as BATCH has ended.
    update_instr_cnt();

    IFDEF(CONFIG_PERF_OPT, update_global());

    Loge("total insts: %'lu, cpu_exec remain: %'li", get_abs_instr_count(), n_remain_total);
  }

  // At this point, instruction counting has been settled.
  // g_nr_guest_instr is the exact number of instructions.

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
    IFDEF(CONFIG_BR_LOG_OUTPUT, br_log_dump());
    Log("nemu: %s\33[0m at pc = " FMT_WORD,
        (nemu_state.state == NEMU_ABORT
             ? "\33[1;31mABORT"
             : (nemu_state.halt_ret == 0 ? "\33[1;32mHIT GOOD TRAP"
                                         : "\33[1;31mHIT BAD TRAP")),
        nemu_state.halt_pc);
    Log("trap code:%d", nemu_state.halt_ret);
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
      break;
    #else // CONFIG_SHARE
      break;
    #endif // CONFIG_SHARE
  }
  pop_context();
}
