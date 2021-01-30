#include <cpu/dccache.h>

DecodeExecState dccache[DCACHE_SIZE + 1] = {};

void dccache_flush() {
  int i;
  for (i = 0; i < DCACHE_SIZE; i ++) {
    dccache[i].pc = 0x1;  // set tag to an invalid pc
    dccache[i].next = &dccache[i + 1];
  }
  dccache[DCACHE_SIZE].pc = 0x1;
  dccache[DCACHE_SIZE].next = NULL;
}
