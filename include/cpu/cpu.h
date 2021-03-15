#ifndef __CPU_CPU_H__
#define __CPU_CPU_H__

#include <common.h>

enum {
  NEMU_EXEC_RUNNING = 0, // unused by longjmp()
  NEMU_EXEC_END,
  NEMU_EXEC_AGAIN,
  NEMU_EXEC_EXCEPTION
};

void cpu_exec(uint64_t n);
__attribute__((noreturn)) void longjmp_exec(int cause);
__attribute__((noreturn)) void longjmp_exception(int ex_cause);

enum {
  MMU_STATE_UPDATE = 1,
  MMU_STATE_FLUSH_TLB = 2,
  MMU_STATE_FLUSH_TCACHE = 4,
};
void set_mmu_state_flag(int flag);

#ifndef CONFIG_PERF_OPT
#define save_globals(s, n)
#else
struct Decode;
void save_globals(struct Decode *s);
#endif

#endif
