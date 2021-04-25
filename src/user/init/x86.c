#include <isa.h>

#ifdef CONFIG_ISA_x86
// we only maintain base of the segment here
uint32_t GDT[4] = {0};

void isa_init_user(word_t sp) {
  cpu.esp = sp;
  cpu.edx = 0; // no handler for atexit()
  cpu.sreg[CSR_CS].val = 0xb; cpu.sreg[CSR_CS].base = 0;
  cpu.sreg[CSR_DS].val = 0xb; cpu.sreg[CSR_DS].base = 0;
  cpu.sreg[CSR_ES].val = 0xb; cpu.sreg[CSR_ES].base = 0;
  cpu.sreg[CSR_FS].val = 0xb; cpu.sreg[CSR_FS].base = 0;
}
#endif
