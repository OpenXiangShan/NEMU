#include "nemu.h"

// this is not consistent with uint8_t
// but it is ok since we do not access the array directly
const uint32_t isa_default_img [] = {
  0x3c048000,  // lui a0, 0x8000
  0xac800000,  // sw  zero, 0(a0)
  0x8c820000,  // lw  v0,0(a0)
  0xf0000000,  // nemu_trap
};
const long isa_default_img_size = sizeof(isa_default_img);

void init_mmu(void);

void init_isa(void) {
  cpu.gpr[0]._32 = 0;
  cpu.pc = PC_START;

  register_pmem(0x80000000u);

  init_mmu();
}
