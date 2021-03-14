#ifndef __CPU_CPU_H__
#define __CPU_CPU_H__

#include <common.h>

void cpu_exec(uint64_t n);
void longjmp_exec(int cause);
void longjmp_exception(int cause);

#ifndef CONFIG_PERF_OPT
#define save_globals(s, n)
#else
struct Decode;
void save_globals(struct Decode *s);
#endif

#endif
