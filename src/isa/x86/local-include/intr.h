#ifndef __X86_INTR_H__
#define __X86_INTR_H__

#include <common.h>

#ifdef CONFIG_PA
#define IRQ_TIMER 32
#else
#define IRQ_TIMER 48
#endif

word_t raise_intr(word_t NO, vaddr_t ret_addr);
#ifndef CONFIG_PA
#define return_on_mem_ex() do { if (cpu.mem_exception != 0) return; } while (0)
#else
#define return_on_mem_ex()
#endif

#endif
