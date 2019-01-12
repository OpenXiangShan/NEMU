#include "nemu.h"

// this is not consistency with uint8_t
// but it is ok since we do not access the array directly
const uint32_t arch_default_img [] = {
  0xac000000,  // sw  zero,0(zero)
  0x8c040000,  // lw  a0,0(zero)
  0xf0000000,  // nemu_trap
};
const long arch_default_img_size = sizeof(arch_default_img);

void init_arch(void) {
  cpu.gpr[0]._32 = 0;
  cpu.pc = PC_START;
}
