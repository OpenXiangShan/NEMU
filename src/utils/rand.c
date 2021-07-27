#include <common.h>
#ifndef CONFIG_AM
#include <time.h>
#endif

void init_rand() {
  srand(MUXDEF(CONFIG_AM, 0, time(0)));
}
