#include <memory/memory.h>
#include <device/map.h>

static uint8_t pmem[PMEM_SIZE] PG_ALIGN = {};

void* guest_to_host(paddr_t addr) { return &pmem[addr]; }
paddr_t host_to_guest(void *addr) { return (void *)pmem - addr; }

static IOMap pmem_map = {
  .name = "pmem",
  .space = pmem,
  .callback = NULL
};

void register_pmem(paddr_t base) {
  pmem_map.low = base;
  pmem_map.high = base + PMEM_SIZE - 1;

  Log("Add '%s' at [0x%08x, 0x%08x]", pmem_map.name, pmem_map.low, pmem_map.high);
}

IOMap* fetch_mmio_map(paddr_t addr);

/* Memory accessing interfaces */

word_t paddr_read(paddr_t addr, int len) {
  if (map_inside(&pmem_map, addr)) {
    paddr_t offset = addr - pmem_map.low;
    return *(uint64_t *)(pmem + offset) & (~0LLu >> ((8 - len) << 3));
  }
  else {
    return map_read(addr, len, fetch_mmio_map(addr));
  }
}

void paddr_write(paddr_t addr, word_t data, int len) {
  if (map_inside(&pmem_map, addr)) {
    paddr_t offset = addr - pmem_map.low;
    memcpy(pmem + offset, &data, len);
  }
  else {
    return map_write(addr, data, len, fetch_mmio_map(addr));
  }
}
