#include <isa.h>

#ifdef CONFIG_ISA_riscv64
void isa_init_user(word_t sp) {
  init_csr_exist();
  cpu.mode = MODE_U;
  cpu.gpr[2]._64 = sp;
  //cpu.edx = 0; // no handler for atexit()
}
#endif
