#include <device/map.h>
#include "../sdi.h"
#include <sys/ipc.h>
#include <sys/shm.h>

volatile uint8_t *tl_base = NULL;

void init_tl() {
  int shmval=shmget(114514,1024,0);
  assert(shmval!=-1);
  tl_base=shmat(shmval,NULL,0);
  assert(tl_base!=(void *)-1);
  add_mmio_map("TileLink", TL_MMIO_BASE, (void *)tl_base, 1024, NULL);
}
