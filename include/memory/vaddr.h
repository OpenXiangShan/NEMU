#ifndef __MEMORY_VADDR_H__
#define __MEMORY_VADDR_H__

#include <common.h>

word_t vaddr_ifetch(vaddr_t addr, int len);
word_t vaddr_read(vaddr_t addr, int len);
void vaddr_write(vaddr_t addr, word_t data, int len);

#define PAGE_SIZE         4096
#define PAGE_MASK         (PAGE_SIZE - 1)
#define PG_ALIGN __attribute((aligned(PAGE_SIZE)))

#endif
