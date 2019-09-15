#include "monitor/monitor.h"
#include "device/map.h"

#define CLINT_MMIO 0x2000000
#define CLINT_MTIMECMP (0x4000 / sizeof(clint_base[0]))
#define CLINT_MTIME    (0xBFF8 / sizeof(clint_base[0]))

static uint64_t *clint_base = NULL;

void clint_intr(void) {
  if (nemu_state.state == NEMU_RUNNING) {
    clint_base[CLINT_MTIME] += 0x800;
  }
}

bool clint_query_intr(void) {
  return clint_base[CLINT_MTIME] >= clint_base[CLINT_MTIMECMP];
}

void init_clint(void) {
  clint_base = (void *)new_space(0x10000);
  add_mmio_map("clint", CLINT_MMIO, (void *)clint_base, 0x10000, NULL);
}
