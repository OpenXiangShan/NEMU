#include "nemu.h"
#include "memory/memory.h"

static inline paddr_t va2pa(vaddr_t addr) {
  return addr & ~0x80000000u;
}

uint32_t isa_vaddr_read(vaddr_t addr, int len) {
  return paddr_read(va2pa(addr), len);
}

void isa_vaddr_write(vaddr_t addr, uint32_t data, int len) {
  paddr_write(va2pa(addr), data, len);
}
