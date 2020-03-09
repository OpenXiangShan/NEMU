#ifndef __MEMORY_PADDR_H__
#define __MEMORY_PADDR_H__

#include <common.h>

#define PMEM_SIZE (256 * 1024 * 1024)

/* convert the guest physical address in the guest program to host virtual address in NEMU */
void* guest_to_host(paddr_t addr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
paddr_t host_to_guest(void *addr);

uint8_t  paddr_read8 (paddr_t addr);
uint16_t paddr_read16(paddr_t addr);
uint32_t paddr_read32(paddr_t addr);
uint64_t paddr_read64(paddr_t addr);
void paddr_write8 (paddr_t addr, uint8_t  data);
void paddr_write16(paddr_t addr, uint16_t data);
void paddr_write32(paddr_t addr, uint32_t data);
void paddr_write64(paddr_t addr, uint64_t data);

static inline uint64_t paddr_read(paddr_t addr, int len) {
  switch (len) {
    case 1: return paddr_read8 (addr);
    case 2: return paddr_read16(addr);
    case 4: return paddr_read32(addr);
#ifdef ISA64
    case 8: return paddr_read64(addr);
#endif
    default: assert(0);
  }
}

static inline void paddr_write(paddr_t addr, uint64_t data, int len) {
  switch (len) {
    case 1: paddr_write8 (addr, data); break;
    case 2: paddr_write16(addr, data); break;
    case 4: paddr_write32(addr, data); break;
#ifdef ISA64
    case 8: paddr_write64(addr, data); break;
#endif
    default: assert(0);
  }
}

#endif
