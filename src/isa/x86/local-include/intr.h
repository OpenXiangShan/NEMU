#ifndef __X86_INTR_H__
#define __X86_INTR_H__

#include <common.h>

#ifdef __PA__
#define IRQ_TIMER 32
#else
#define IRQ_TIMER 48
#endif

word_t raise_intr(word_t NO, vaddr_t ret_addr);

#endif
