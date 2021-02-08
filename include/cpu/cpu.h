#ifndef __CPU_CPU_H__
#define __CPU_CPU_H__

#ifdef CONFIG_DEBUG
void debug_hook(vaddr_t pc, int len);
#else
static inline void debug_hook(vaddr_t pc, int len) {}
#endif

void cpu_exec(uint64_t n);

#endif
