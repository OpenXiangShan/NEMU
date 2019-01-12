#include "nemu.h"

uint32_t arch_vaddr_read(vaddr_t addr, int len) {
  return paddr_read(addr, len);
}

void arch_vaddr_write(vaddr_t addr, uint32_t data, int len) {
  paddr_write(addr, data, len);
}
