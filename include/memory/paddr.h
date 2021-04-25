#ifndef __MEMORY_PADDR_H__
#define __MEMORY_PADDR_H__

#include <common.h>

#ifdef CONFIG_MODE_USER
#define CONFIG_MBASE 0
#define CONFIG_MSIZE 0
#define CONFIG_PC_RESET_OFFSET 0
#endif

#define RESET_VECTOR (CONFIG_MBASE + CONFIG_PC_RESET_OFFSET)

/* convert the guest physical address in the guest program to host virtual address in NEMU */
void* guest_to_host(paddr_t paddr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
paddr_t host_to_guest(void *haddr);

static inline bool in_pmem(paddr_t addr) {
  paddr_t mask = CONFIG_MSIZE - 1;
  return (addr & ~mask) == CONFIG_MBASE;
}

word_t paddr_read(paddr_t addr, int len);
void paddr_write(paddr_t addr, int len, word_t data);

#endif
