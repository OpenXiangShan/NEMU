#ifndef __MEMORY_VADDR_H__
#define __MEMORY_VADDR_H__

#include <common.h>

struct Decode;
word_t vaddr_ifetch(vaddr_t addr, int len);
word_t vaddr_read(struct Decode *s, vaddr_t addr, int len, int mmu_mode);
void vaddr_write(struct Decode *s, vaddr_t addr, int len, word_t data, int mmu_mode);

word_t vaddr_read_safe(vaddr_t addr, int len);

#define PAGE_SHIFT        12
#define PAGE_SIZE         (1ul << PAGE_SHIFT)
#define PAGE_MASK         (PAGE_SIZE - 1)

int force_raise_pf(vaddr_t vaddr, int type);

#endif
