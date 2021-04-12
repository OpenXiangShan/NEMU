#ifndef __MIPS32_INTR_H__
#define __MIPS32_INTR_H__

#include <common.h>

#define EX_SYSCALL 8
#define EX_TLB_LD 2
#define EX_TLB_ST 3
#define TLB_REFILL 0x80

#define MEM_OK 0

word_t raise_intr(uint32_t NO, vaddr_t epc);

#endif
