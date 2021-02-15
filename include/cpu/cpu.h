#ifndef __CPU_CPU_H__
#define __CPU_CPU_H__

#ifdef CONFIG_DEBUG
void debug_hook(vaddr_t pc, const char *asmbuf);
#else
static inline void debug_hook(vaddr_t pc, const char *asmbuf) {}
#endif

void cpu_exec(uint64_t n);

#endif
