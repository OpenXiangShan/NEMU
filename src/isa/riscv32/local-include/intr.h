#ifndef __RISCV32_INTR_H__
#define __RISCV32_INTR_H__

#include <common.h>

word_t raise_intr(uint32_t NO, vaddr_t epc);

#endif
