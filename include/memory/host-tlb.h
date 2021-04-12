#ifndef __MEMORY_HOST_TLB_H__
#define __MEMORY_HOST_TLB_H__

#include <common.h>

struct Decode;
word_t hosttlb_read(struct Decode *s, vaddr_t vaddr, int len, int type);
void hosttlb_write(struct Decode *s, vaddr_t vaddr, int len, word_t data);
void hosttlb_init();
void hosttlb_flush(vaddr_t vaddr);

#endif
