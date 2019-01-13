#include "memory/memory.h"
#include "device/map.h"

#define IO_SPACE_MAX (1024 * 1024)

static uint8_t io_space[IO_SPACE_MAX] PG_ALIGN;
static uint8_t *p_space = io_space;

uint8_t* new_space(int size) {
  uint8_t *p = p_space;
  // page aligned;
  size = (size + (PAGE_SIZE - 1)) & ~PAGE_MASK;
  p_space += size;
  assert(p_space - io_space < IO_SPACE_MAX);
  return p;
}

uint32_t map_read(paddr_t addr, int len, IOMap *map) {
  assert(len >= 1 && len <= 4);
  uint32_t offset = addr - map->low;
  invoke_callback(map->callback, offset, len, false); // prepare data to read

  uint32_t data = *(uint32_t *)(map->space + offset) & (~0u >> ((4 - len) << 3));
  return data;
}

void map_write(paddr_t addr, int len, uint32_t data, IOMap *map) {
  assert(len >= 1 && len <= 4);
  uint32_t offset = addr - map->low;

  uint8_t *p = map->space + offset;
  uint8_t *p_data = (uint8_t *)&data;
  switch (len) {
    case 4: p[3] = p_data[3];
    case 3: p[2] = p_data[2];
    case 2: p[1] = p_data[1];
    case 1: p[0] = p_data[0]; break;
  }

  invoke_callback(map->callback, offset, len, true);
}
