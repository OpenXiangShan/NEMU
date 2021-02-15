#ifndef __CPU_CPU_H__
#define __CPU_CPU_H__

#include <common.h>

void cpu_exec(uint64_t n);
void longjmp_exec(int cause);

#ifdef CONFIG_PERF_OPT
void save_globals(vaddr_t pc, uint32_t n);
#else
#define save_globals(pc, n)
#endif

#endif
