#include <cpu/dccache.h>

dccacheEntry dccache[DCACHE_SIZE] = {};

void dccache_flush() {
  int i;
  for (i = 0; i < DCACHE_SIZE; i ++) {
    dccache[i].tag = 0x1;  // set tag to an invalid pc
  }
}
