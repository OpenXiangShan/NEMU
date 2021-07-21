#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include <cpu/cpu.h>

bool vp_enable() {
  return MUXDEF(CONFIG_MODE_USER, true, mstatus->vs != 0);
}

void vp_set_dirty() {
  // lazily update
  mstatus->vs = 3;
}
