#ifndef __MEMORY_VADDR_H__
#define __MEMORY_VADDR_H__

#include <common.h>

uint8_t  vaddr_ifetch8 (vaddr_t addr);
uint16_t vaddr_ifetch16(vaddr_t addr);
uint32_t vaddr_ifetch32(vaddr_t addr);
uint64_t vaddr_ifetch64(vaddr_t addr);
uint8_t  vaddr_read8 (vaddr_t addr);
uint16_t vaddr_read16(vaddr_t addr);
uint32_t vaddr_read32(vaddr_t addr);
uint64_t vaddr_read64(vaddr_t addr);
void vaddr_write8 (vaddr_t addr, uint8_t  data);
void vaddr_write16(vaddr_t addr, uint16_t data);
void vaddr_write32(vaddr_t addr, uint32_t data);
void vaddr_write64(vaddr_t addr, uint64_t data);

static inline uint64_t vaddr_ifetch(vaddr_t addr, int len) {
  switch (len) {
    case 1: return vaddr_ifetch8 (addr);
    case 2: return vaddr_ifetch16(addr);
    case 4: return vaddr_ifetch32(addr);
#ifdef ISA64
    case 8: return vaddr_ifetch64(addr);
#endif
    default: assert(0);
  }
}

static inline uint64_t vaddr_read(vaddr_t addr, int len) {
  switch (len) {
    case 1: return vaddr_read8 (addr);
    case 2: return vaddr_read16(addr);
    case 4: return vaddr_read32(addr);
#ifdef ISA64
    case 8: return vaddr_read64(addr);
#endif
    default: assert(0);
  }
}

static inline void vaddr_write(vaddr_t addr, uint64_t data, int len) {
  switch (len) {
    case 1: vaddr_write8 (addr, data); break;
    case 2: vaddr_write16(addr, data); break;
    case 4: vaddr_write32(addr, data); break;
#ifdef ISA64
    case 8: vaddr_write64(addr, data); break;
#endif
    default: assert(0);
  }
}

#define PAGE_SIZE         4096
#define PAGE_MASK         (PAGE_SIZE - 1)
#define PG_ALIGN __attribute((aligned(PAGE_SIZE)))

#endif
