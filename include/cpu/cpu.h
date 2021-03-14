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
void longjmp_exec(int cause);
void longjmp_exception(int ex_cause);
void set_system_state_update_flag();

#ifndef CONFIG_PERF_OPT
#define save_globals(s, n)
#else
struct Decode;
void save_globals(struct Decode *s);
#endif

#endif
