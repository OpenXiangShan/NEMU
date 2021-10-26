#include <common.h>

uint32_t bit_scan(uint32_t x, bool reverse) {
  if (x == 0) return 32;
  int bit  = (reverse ? 31 : 0);
  int step = (reverse ? -1 : 1);
  while ((x & (1u << bit)) == 0) bit += step;
  return bit;
}
