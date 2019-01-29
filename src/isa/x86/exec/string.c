#include "cpu/exec.h"

make_EHelper(movs) {
  rtl_lm(&s0, &cpu.esi, id_dest->width);
  rtl_sm(&cpu.edi, &s0, id_dest->width);

  rtl_addi(&cpu.esi, &cpu.esi, id_dest->width);
  rtl_addi(&cpu.edi, &cpu.edi, id_dest->width);

  print_asm("movs (%%esi), (%%edi)");
}
