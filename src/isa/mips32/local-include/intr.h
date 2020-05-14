#ifndef __INTR_H__
#define __INTR_H__

#include <cpu/decode.h>

#define EX_SYSCALL 8
#define EX_TLB_LD 2
#define EX_TLB_ST 3
#define TLB_REFILL 0x80

#define MEM_OK 0

void raise_intr(DecodeExecState *s, uint32_t NO, vaddr_t epc);
#define return_on_mem_ex() do { if (cpu.mem_exception != MEM_OK) return; } while (0)

#endif
