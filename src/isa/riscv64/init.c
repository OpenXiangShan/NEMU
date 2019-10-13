#include "nemu.h"
#include "csr.h"

// this is not consistent with uint8_t
// but it is ok since we do not access the array directly
const uint32_t isa_default_img [] = {
  0x800002b7,  // lui t0,0x80000
  0x0002a023,  // sw  zero,0(t0)
  0x0002a503,  // lw  a0,0(t0)
  0x0000006b,  // nemu_trap
};
const long isa_default_img_size = sizeof(isa_default_img);

void init_clint(void);

void init_isa(void) {
  cpu.gpr[0]._64 = 0;
  cpu.pc = PC_START;
  //mstatus->val = 0x000c0100;

  register_pmem(0x80000000u);

  init_clint();
}
