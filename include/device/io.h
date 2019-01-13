#ifndef __IO_H__
#define __IO_H__

#include "common.h"

int is_mmio(paddr_t);
uint32_t mmio_read(paddr_t, int, int);
void mmio_write(paddr_t, int, uint32_t, int);

#endif
