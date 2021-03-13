#ifndef __MEMORY_PADDR_H__
#define __MEMORY_PADDR_H__

#include <common.h>

#define RESET_VECTOR (CONFIG_MBASE + CONFIG_PC_RESET_OFFSET)

/* convert the guest physical address in the guest program to host virtual address in NEMU */
void* guest_to_host(paddr_t paddr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
paddr_t host_to_guest(void *haddr);

word_t paddr_read(paddr_t addr, int len);
void paddr_write(paddr_t addr, int len, word_t data);

#endif
