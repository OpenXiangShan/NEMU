#include "common.h"
#include "device/map.h"

#define NR_MAP 8

static IOMap maps[NR_MAP];
static int nr_map = 0;

/* device interface */
void add_mmio_map(paddr_t addr, uint8_t* space, int len, io_callback_t callback) {
  assert(nr_map < NR_MAP);
  maps[nr_map] = (IOMap){ .low = addr, .high = addr + len - 1,
    .space = space, .callback = callback };
  nr_map ++;
}

/* bus interface */
int is_mmio(paddr_t addr) {
  return find_mapid_by_addr(maps, nr_map, addr);
}

uint32_t mmio_read(paddr_t addr, int len, int mapid) {
  assert(mapid != -1);
  return map_read(addr, len, &maps[mapid]);
}

void mmio_write(paddr_t addr, int len, uint32_t data, int mapid) {
  assert(mapid != -1);
  map_write(addr, len, data, &maps[mapid]);
}
