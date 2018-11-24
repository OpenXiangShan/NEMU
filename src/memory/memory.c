#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int mmio_id = is_mmio(addr);
  if (mmio_id != -1) {
    return mmio_read(addr, len, mmio_id) & (~0u >> ((4 - len) << 3));
  }

  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, uint32_t data, int len) {
  int mmio_id = is_mmio(addr);
  if (mmio_id != -1) {
    mmio_write(addr, len, data, mmio_id);
    return;
  }

  memcpy(guest_to_host(addr), &data, len);
}

paddr_t page_translate(vaddr_t addr, bool is_write);

uint32_t vaddr_read(vaddr_t addr, int len) {
  uint32_t data;
  if(cpu.cr0.paging) {
    paddr_t paddr = page_translate(addr, false);
    uint32_t remain_byte = PAGE_SIZE - (addr & PAGE_MASK);
    if(remain_byte < len) {
      /* data cross the page boundary */
      data = paddr_read(paddr, remain_byte);

      paddr = page_translate(addr + remain_byte, false);
      data |= paddr_read(paddr, len - remain_byte) << (remain_byte << 3);
    }
    else {
      data = paddr_read(paddr, len);
    }
  }
  else {
    data = paddr_read(addr, len);
  }

  return data;
}

void vaddr_write(vaddr_t addr, uint32_t data, int len) {
  if(cpu.cr0.paging) {
    paddr_t paddr = page_translate(addr, true);
    uint32_t remain_byte = PAGE_SIZE - (addr & PAGE_MASK);
    if(remain_byte < len) {
      /* data cross the page boundary */
      uint32_t cut = PAGE_SIZE - (addr & PAGE_MASK);
      assert(cut < 4);
      paddr_write(paddr, data, cut);

      paddr = page_translate(addr + cut, true);
      paddr_write(paddr, data >> (cut << 3), len - cut);
    }
    else {
      paddr_write(paddr, data, len);
    }
  }
  else {
    paddr_write(addr, data, len);
  }
}
