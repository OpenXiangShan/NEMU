/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __CPU_CPU_H__
#define __CPU_CPU_H__

#include <common.h>
#include <setjmp.h>
#include <stdarg.h>

#define AHEAD_LENGTH 500

enum {
  NEMU_EXEC_RUNNING = 0, // unused by longjmp()
  NEMU_EXEC_END,
  NEMU_EXEC_AGAIN,
  NEMU_EXEC_EXCEPTION
};

uint64_t get_abs_instr_count();
uint64_t get_abs_instr_count_csr();

void cpu_exec(uint64_t n);

#define CONTEXT_STACK_SIZE 5
extern int context_idx;
extern jmp_buf context_stack[CONTEXT_STACK_SIZE];
#define PUSH_CONTEXT(cause) \
do{ \
  if (context_idx + 1 >= CONTEXT_STACK_SIZE) { \
    panic("Unexcepted exeception context idx = %d", context_idx); \
  } \
  context_idx++; \
  *cause = setjmp(context_stack[context_idx]); \
} while (0)

void pop_context();
__attribute__((noreturn)) void longjmp_context(int cause);
__attribute__((noreturn)) void longjmp_exception(int ex_cause);

enum {
  SYS_STATE_UPDATE = 1,
  SYS_STATE_FLUSH_TCACHE = 2,
};
void set_sys_state_flag(int flag);
// The block cache is flushed at the end of the current instruction because
// decoded code or address mappings may have changed; blocks kept across
// context switches (CONFIG_TCACHE_KEEP_ON_TRAP) are discarded as well.
void tcache_flush_code_changed(void);
#ifdef CONFIG_TCACHE_KEEP_ON_TRAP
void tcache_invalidate(void);
// Address mappings may have changed, not code: kept blocks are checked
// before they run again, and discarded where their code moved.
void tcache_mappings_changed(void);
// Flushes the TLBs for a new address space: kept blocks, tagged with it, stay.
void mmu_tlb_flush_new_space(void);
#endif
// Execution modes. Exact: the reference code paths only (its block cache and
// host TLB). Follower: the interpreter and softfloat on the kept block cache.
void cpu_enter_exact_mode(void);
void cpu_enter_follower_mode(void);
// Leader: the fast path (host floating point).
void cpu_enter_leader_mode(void);
// Writes the instruction count, pc and cause of every execution batch to path.
void cpu_set_batch_trace(const char *path);
void mmu_tlb_flush(vaddr_t vaddr);
// SFENCE.VMA: as mmu_tlb_flush(), but a global fence lets kept blocks stay,
// checked again before they run (tcache_mappings_changed()).
void mmu_tlb_flush_fence(vaddr_t vaddr);
#ifdef CONFIG_RVH
void mmu_tlb_flush_host(vaddr_t vaddr);
void mmu_tlb_flush_host_fence(vaddr_t vaddr);
void mmu_tlb_flush_guest(vaddr_t vaddr);
#endif

struct Decode;
void save_globals(struct Decode *s);
void fetch_decode(struct Decode *s, vaddr_t pc);
void lightqs_take_reg_snapshot();
void clint_take_snapshot();
void lightqs_take_spec_reg_snapshot();
void clint_take_spec_snapshot();
uint64_t lightqs_restore_reg_snapshot(uint64_t n);
#ifdef CONFIG_LIGHTQS
void pmem_record_restore(uint64_t restore_inst_cnt);
#endif
void clint_restore_snapshot(uint64_t restore_inst_cnt);

struct lightqs_reg_ss {
  uint64_t inst_cnt;
  uint64_t br_cnt;
  // snapshot stores GPR CSR
  uint64_t mstatus, mcause, mepc, sstatus, scause, sepc,
  satp, mip, mie, mscratch, sscratch, mideleg, medeleg,
  mtval, stval, mtvec, stvec;
#ifdef CONFIG_RV_MBMC
  uint64_t mbmc;
#endif
#ifdef CONFIG_RV_MPT_CHECK
  uint64_t mmpt;
#endif
#ifdef CONFIG_RVV
  uint64_t vtype, vstart, vxsat, vxrm, vl;
#endif // CONFIG_RVV
  uint64_t gpr[32], fpr[32];
  uint64_t mode;
  uint64_t pc;
  uint64_t lr_addr, lr_valid;
  // RAM is store-logged at another position

  int ifetch_mmu_state;
  int data_mmu_state;
};

extern unsigned ref_hartid;
#ifdef CONFIG_SHARE
static inline void ref_log_cpu_impl(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  fprintf(stderr, "[NEMU][%u] ", ref_hartid);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  va_end(ap);
}
#define ref_log_cpu(...) \
  do { \
    if (unlikely(dynamic_config.debug_difftest)) { \
      ref_log_cpu_impl(__VA_ARGS__); \
    } \
  } while (0)
#else
static inline void ref_log_cpu(const char *fmt, ...) {
  (void)fmt;
}
#endif

#endif
