#include "nemu.h"

const uint8_t arch_default_img [] = {
};
const long arch_default_img_size = sizeof(arch_default_img);

void init_arch(void) {
  cpu.gpr[0]._32 = 0;
  cpu.pc = PC_START;
}
