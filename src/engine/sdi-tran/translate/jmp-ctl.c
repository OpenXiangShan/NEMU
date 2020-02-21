#include <device/map.h>
#include "../sdi.h"
#include <sys/ipc.h>
#include <sys/shm.h>

volatile uint8_t *jmp_base = NULL;

void init_jmp() {
  int shmval=shmget(1096,1024,0);
  assert(shmval!=-1);
  jmp_base=shmat(shmval,NULL,0);
  assert(jmp_base!=(void *)-1);
  add_mmio_map("Jmp_ctl", JMP_MMIO_BASE, (void *)jmp_base, 1024, NULL);
}
