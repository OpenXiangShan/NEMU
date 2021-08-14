#ifndef __X86_INTR_H__
#define __X86_INTR_H__

#include <common.h>

#ifdef CONFIG_PA
#define IRQ_TIMER 32
#define return_on_mem_ex()
#else
#define IRQ_TIMER 48
#define return_on_mem_ex() do { if (cpu.mem_exception != 0) return; } while (0)
#endif

#endif
