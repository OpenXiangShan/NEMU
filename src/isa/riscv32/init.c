#include "nemu.h"

// this is not consistent with uint8_t
// but it is ok since we do not access the array directly
const uint32_t isa_default_img [] = {
  0x00002023,  // sw  zero, 0(zero)
  0x00002503,  // lw  a0,0(zero)
  0x0000006b,  // nemu_trap
};
const long isa_default_img_size = sizeof(isa_default_img);

void init_isa(void) {
  cpu.gpr[0]._32 = 0;
  cpu.pc = PC_START;
}
