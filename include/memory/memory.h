#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <isa.h>

#define PMEM_SIZE (256 * 1024 * 1024)

/* convert the guest physical address in the guest program to host virtual address in NEMU */
void* guest_to_host(paddr_t addr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
paddr_t host_to_guest(void *addr);

void register_pmem(paddr_t base);

#define vaddr_read isa_vaddr_read
#define vaddr_write isa_vaddr_write

word_t paddr_read(paddr_t, int);
void paddr_write(paddr_t, word_t, int);

#define PAGE_SIZE         4096
#define PAGE_MASK         (PAGE_SIZE - 1)
#define PG_ALIGN __attribute((aligned(PAGE_SIZE)))

#endif
