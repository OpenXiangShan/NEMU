#include "../local-include/csr.h"

bool fp_enable() {
  return (mstatus->fs != 0);
}

void fp_set_dirty() {
  mstatus->sd = 1;
  mstatus->fs = 3;
}
