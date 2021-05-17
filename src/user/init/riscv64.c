#include <isa.h>

#ifdef CONFIG_ISA_riscv64
void init_csr();

void isa_init_user(word_t sp) {
  init_csr();
  cpu.mode = MODE_U;
  cpu.gpr[2]._64 = sp;
  //cpu.edx = 0; // no handler for atexit()
}
#endif
