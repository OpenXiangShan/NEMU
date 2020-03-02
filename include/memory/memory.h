#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <isa.h>

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

static inline uint64_t vaddr_read(vaddr_t addr, int len) {
  switch (len) {
    case 1: return isa_vaddr_read8 (addr);
    case 2: return isa_vaddr_read16(addr);
    case 4: return isa_vaddr_read32(addr);
    case 8: return isa_vaddr_read64(addr);
    default: assert(0);
  }
}

static inline void vaddr_write(vaddr_t addr, uint64_t data, int len) {
  switch (len) {
    case 1: isa_vaddr_write8 (addr, data); break;
    case 2: isa_vaddr_write16(addr, data); break;
    case 4: isa_vaddr_write32(addr, data); break;
    case 8: isa_vaddr_write64(addr, data); break;
    default: assert(0);
  }
}

static inline uint64_t paddr_read(paddr_t addr, int len) {
  switch (len) {
    case 1: return paddr_read8 (addr);
    case 2: return paddr_read16(addr);
    case 4: return paddr_read32(addr);
    case 8: return paddr_read64(addr);
    default: assert(0);
  }
}

static inline void paddr_write(paddr_t addr, uint64_t data, int len) {
  switch (len) {
    case 1: paddr_write8 (addr, data); break;
    case 2: paddr_write16(addr, data); break;
    case 4: paddr_write32(addr, data); break;
    case 8: paddr_write64(addr, data); break;
    default: assert(0);
  }
}

#define PAGE_SIZE         4096
#define PAGE_MASK         (PAGE_SIZE - 1)
#define PG_ALIGN __attribute((aligned(PAGE_SIZE)))

#endif
