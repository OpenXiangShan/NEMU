#include "nemu.h"

// this is not consistency with uint8_t
// but it is ok since we do not access the array directly
const uint32_t isa_default_img [] = {
  0xac000000,  // sw  zero,0(zero)
  0x8c020000,  // lw  v0,0(zero)
  0xf0000000,  // nemu_trap
};
const long isa_default_img_size = sizeof(isa_default_img);

void init_isa(void) {
  cpu.gpr[0]._32 = 0;
  cpu.pc = PC_START;
}
