#ifndef __PADDR_H__
#define __PADDR_H__

#include <common.h>

word_t paddr_ifetch(paddr_t addr, int len);
word_t paddr_read(paddr_t addr, int len);
void paddr_write(paddr_t addr, word_t data, int len);

#endif
