#include "monitor/monitor.h"
#include "device/map.h"

#define CLINT_MMIO 0x2000000

static uint64_t *clint_base = NULL;

void init_clint(void) {
  clint_base = (void *)new_space(0x10000);
  add_mmio_map("clint", CLINT_MMIO, (void *)clint_base, 0x10000, NULL);
}
