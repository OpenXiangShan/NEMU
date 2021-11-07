#include <isa.h>

void init_csr();

void isa_init_user(word_t sp) {
  init_csr(); // initialize CSRs for floating point
  cpu.gpr[2] = sp;
}
