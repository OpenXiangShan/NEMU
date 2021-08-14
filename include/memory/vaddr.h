#ifndef __MEMORY_VADDR_H__
#define __MEMORY_VADDR_H__

#include <common.h>

word_t vaddr_ifetch(vaddr_t addr, int len);
#ifndef __ICS_EXPORT
struct Decode;
word_t vaddr_read(struct Decode *s, vaddr_t addr, int len, int mmu_mode);
void vaddr_write(struct Decode *s, vaddr_t addr, int len, word_t data, int mmu_mode);
word_t vaddr_read_safe(vaddr_t addr, int len);
#else
word_t vaddr_read(vaddr_t addr, int len);
void vaddr_write(vaddr_t addr, int len, word_t data);
#endif

#define PAGE_SHIFT        12
#define PAGE_SIZE         (1ul << PAGE_SHIFT)
#define PAGE_MASK         (PAGE_SIZE - 1)

#endif
