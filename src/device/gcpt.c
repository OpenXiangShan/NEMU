#include "device/map.h"
#include <utils.h>

#define CONFIG_GCPT_MMIO 50000000
#define GCPT_MEM_SIZE (128 * 1024 * 1024)

static uint8_t *gcpt_host_address = NULL;

// read gcpt memory do not need any handle, just resturn data
static void gcpt_io_handler(uint32_t offset, int len, bool is_write) {}

struct {
  uint8_t *mmio_address;
  uint8_t *host_address;
  uint64_t size;
} gcpt_info = {
  .size = GCPT_MEM_SIZE,
  .host_address = 0,
  .mmio_address = (uint8_t *)CONFIG_GCPT_MMIO,
};

uint8_t *get_gcpt_mmio_base() {
  return gcpt_info.host_address;
}

void init_gcpt() {
  gcpt_host_address = new_space(gcpt_info.size);
  gcpt_info.host_address=gcpt_host_address;
  add_mmio_map("gcpt", (uint64_t)gcpt_info.mmio_address, gcpt_info.host_address, gcpt_info.size, gcpt_io_handler);
}
