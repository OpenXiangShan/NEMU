#include <cpu/dccache.h>

DecodeExecState dccache[DCACHE_SIZE] = {};

void dccache_flush() {
  int i;
  for (i = 0; i < DCACHE_SIZE; i ++) {
    dccache[i].pc = 0x1;  // set tag to an invalid pc
  }
}
