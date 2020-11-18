#include <monitor/monitor.h>
#include <device/alarm.h>
#include <device/map.h>
#include "local-include/csr.h"

#define CLINT_MTIMECMP (0x4000 / sizeof(clint_base[0]))
#define CLINT_MTIME    (0xBFF8 / sizeof(clint_base[0]))

static uint64_t *clint_base = NULL;

static inline void update_mtip(void) {
  mip->mtip = (clint_base[CLINT_MTIME] >= clint_base[CLINT_MTIMECMP]);
}

void clint_intr(void) {
  if (nemu_state.state == NEMU_RUNNING) {
    clint_base[CLINT_MTIME] += 0x800;
    update_mtip();
  }
}

static void clint_io_handler(uint32_t offset, int len, bool is_write) {
  update_mtip();
}

void init_clint(void) {
  clint_base = (void *)new_space(0x10000);
  add_mmio_map("clint", CLINT_MMIO, (void *)clint_base, 0x10000, clint_io_handler);
  add_alarm_handle(clint_intr);
}
