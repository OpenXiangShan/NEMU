#ifndef __CPU_CPU_H__
#define __CPU_CPU_H__

#include <common.h>

#ifndef __ICS_EXPORT
enum {
  NEMU_EXEC_RUNNING = 0, // unused by longjmp()
  NEMU_EXEC_END,
  NEMU_EXEC_AGAIN,
  NEMU_EXEC_EXCEPTION
};

void cpu_exec(uint64_t n);
IFDEF(CONFIG_PERF_OPT, __attribute__((noreturn))) void longjmp_exec(int cause);
IFDEF(CONFIG_PERF_OPT, __attribute__((noreturn))) void longjmp_exception(int ex_cause);

enum {
  SYS_STATE_UPDATE = 1,
  SYS_STATE_FLUSH_TCACHE = 2,
};
void set_sys_state_flag(int flag);
void mmu_tlb_flush(vaddr_t vaddr);

struct Decode;
void save_globals(struct Decode *s);
void fetch_decode(struct Decode *s, vaddr_t pc);
#else
void cpu_exec(uint64_t n);
#endif

void set_nemu_state(int state, vaddr_t pc, int halt_ret);
void invalid_instr(vaddr_t thispc);
#define NEMUTRAP(thispc, code) do { \
  difftest_skip_ref(); \
  set_nemu_state(NEMU_END, thispc, code); \
} while (0)
#define INV(thispc) invalid_instr(thispc)

#define check_ex(ret) IFNDEF(CONFIG_PERF_OPT, do { \
  extern word_t g_ex_cause; \
  if (g_ex_cause != NEMU_EXEC_RUNNING) { return ret; } \
} while (0))

#endif
