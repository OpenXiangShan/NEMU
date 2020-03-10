#include <device/map.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "jmp-ctl.h"

volatile uint8_t *jmp_base = NULL;

void init_jmp() {
  int shmval = shmget(1096, JMP_MMIO_SIZE, IPC_CREAT | 0666);
  assert(shmval != -1);
  jmp_base = shmat(shmval,NULL,0);
  assert(jmp_base != (void *)-1);
  memset((void *)jmp_base, 0, JMP_MMIO_SIZE);
  add_mmio_map("Jmp_ctl", JMP_MMIO_BASE, (void *)jmp_base, JMP_MMIO_SIZE, NULL);
}
