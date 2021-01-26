#ifndef __CPU_CPU_EXEC_H__
#define __CPU_CPU_EXEC_H__

#include <utils.h>
#include <cpu/difftest.h>

#ifdef DEBUG
void debug_hook(vaddr_t pc, int len);
#else
static inline void debug_hook(vaddr_t pc, int len) {}
#endif

// We use macro to improve performance.
// Note that this macro should be put inside the execution loop.
#define cpu_exec_2nd_part(pc, snpc, npc) { \
  debug_hook(pc, snpc - pc); \
  difftest_step(pc, npc); \
  if (nemu_state.state != NEMU_RUNNING) break; \
}

#endif
