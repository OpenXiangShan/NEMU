#include <device/map.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "fake-tl.h"

volatile uint8_t *tl_base = NULL;

void init_tl() {
  int shmval = shmget(114514, TL_MMIO_SIZE, IPC_CREAT|0666);
  assert(shmval != -1);
  tl_base = shmat(shmval,NULL,0);
  assert(tl_base != (void *)-1);
  memset((void *)tl_base, 0, TL_MMIO_SIZE);
  add_mmio_map("TileLink", TL_MMIO_BASE, (void *)tl_base, TL_MMIO_SIZE, NULL);

  *tl_base = 1;
  while (*tl_base) ; // wait unitl clear
}
