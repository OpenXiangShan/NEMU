#include <cpu/dccache.h>

DecodeExecState dccache[DCACHE_SIZE + 1] = {};

void dccache_flush() {
  int i;
  for (i = 0; i < DCACHE_SIZE + 1; i ++) {
    dccache[i].pc = dccache[i].snpc = 0x1;  // set tag to an invalid pc
  }
}
