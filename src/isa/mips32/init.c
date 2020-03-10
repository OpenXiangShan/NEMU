#include <isa.h>
#include <memory/paddr.h>

// this is not consistent with uint8_t
// but it is ok since we do not access the array directly
static const uint32_t img [] = {
  0x3c048000,  // lui a0, 0x8000
  0xac800000,  // sw  zero, 0(a0)
  0x8c820000,  // lw  v0,0(a0)
  0xf0000000,  // nemu_trap
};

static void restart() {
  cpu.gpr[0]._32 = 0;
  cpu.pc = PMEM_BASE + IMAGE_START;
}

void init_isa(void) {
  /* Load built-in image. */
  memcpy(guest_to_host(IMAGE_START), img, sizeof(img));

  /* Initialize this virtual computer system. */
  restart();

  void init_mmu(void);
  init_mmu();
}
