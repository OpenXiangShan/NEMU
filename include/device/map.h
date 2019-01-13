#ifndef __MAP_H__
#define __MAP_H__

#include "common.h"

typedef void(*io_callback_t)(uint32_t, int, bool);
uint8_t* new_space(int size);

typedef struct {
  // we cheat ioaddr_t as paddr_t here
  paddr_t low;
  paddr_t high;
  uint8_t *space;
  io_callback_t callback;
} IOMap;

static inline int find_mapid_by_addr(IOMap *maps, int size, paddr_t addr) {
  int i;
  for (i = 0; i < size; i ++) {
    if (addr >= maps[i].low && addr <= maps[i].high) {
      return i;
    }
  }
  return -1;
}

static inline void invoke_callback(io_callback_t c, uint32_t offset, int len, bool is_write) {
  if (c != NULL) {
    c(offset, len, is_write);
  }
}


void add_pio_map(ioaddr_t addr, uint8_t *space, int len, io_callback_t callback);
void add_mmio_map(paddr_t addr, uint8_t* space, int len, io_callback_t callback);

uint32_t map_read(paddr_t addr, int len, IOMap *map);
void map_write(paddr_t addr, int len, uint32_t data, IOMap *map);

#endif
