#include "../local-include/mmu.h"

static inline def_EHelper(movs) {
#ifndef __ENGINE_interpreter__
  panic("not support in engines other than interpreter");
#endif

  word_t count = (s->isa.rep_flags ? cpu.ecx : 1);
  if (count != 0) {
    rtl_lm(s, s0, &cpu.esi, 0, id_dest->width);
    return_on_mem_ex();
    rtl_sm(s, &cpu.edi, 0, s0, id_dest->width);
    return_on_mem_ex();

    rtl_addi(s, &cpu.esi, &cpu.esi, (cpu.DF ? -1 : 1) * id_dest->width);
    rtl_addi(s, &cpu.edi, &cpu.edi, (cpu.DF ? -1 : 1) * id_dest->width);
  }
  if (s->isa.rep_flags && count != 0) {
    cpu.ecx --;
    if (count - 1 != 0) rtl_j(s, cpu.pc);
  }

  print_asm("movs (%%esi), (%%edi)");
}

static inline def_EHelper(lods) {
  rtl_lm(s, ddest, &cpu.esi, 0, id_dest->width);
  rtl_addi(s, &cpu.esi, &cpu.esi, (cpu.DF ? -1 : 1) * id_dest->width);
  operand_write(s, id_dest, ddest);

  print_asm("lods (%%esi), %%eax");
}

static inline def_EHelper(stos) {
#ifndef __ENGINE_interpreter__
  panic("not support in engines other than interpreter");
#endif

  word_t count = (s->isa.rep_flags ? cpu.ecx : 1);
  if (count != 0) {
    rtl_sm(s, &cpu.edi, 0, dsrc1, id_dest->width);
    return_on_mem_ex();
    rtl_addi(s, &cpu.edi, &cpu.edi, (cpu.DF ? -1 : 1) * id_dest->width);
  }
  if (s->isa.rep_flags && count != 0) {
    cpu.ecx --;
    if (count - 1 != 0) rtl_j(s, cpu.pc);
  }

  print_asm("stos %%eax, (%%edi)");
}

static inline def_EHelper(scas) {
#ifndef __ENGINE_interpreter__
  panic("not support in engines other than interpreter");
#endif

  int is_repnz = (s->isa.rep_flags == PREFIX_REPNZ);
  word_t count = (s->isa.rep_flags ? cpu.ecx : 1);
  if (count != 0) {
    rtl_lm(s, s0, &cpu.edi, 0, id_dest->width);
    rtl_setrelop(s, RELOP_EQ, s1, s0, dsrc1);
    rtl_set_ZF(s, s1);
    rtl_addi(s, &cpu.edi, &cpu.edi, (cpu.DF ? -1 : 1) * id_dest->width);
  }
  if (s->isa.rep_flags && count != 0) {
    cpu.ecx --;
    if ((count - 1 != 0) && (is_repnz ^ cpu.ZF)) rtl_j(s, cpu.pc);
  } else {
    rtl_sub(s, s1, dsrc1, s0);
    rtl_update_ZFSF(s, s1, id_dest->width);
    rtl_is_sub_carry(s, s1, dsrc1, s0);
    rtl_set_CF(s, s1);
    rtl_sub(s, s1, dsrc1, s0);
    rtl_is_sub_overflow(s, s1, s1, dsrc1, s0, id_dest->width);
    rtl_set_OF(s, s1);
  }

  print_asm("stos %%eax, (%%edi)");
}

static inline def_EHelper(cmps) {
#ifndef __ENGINE_interpreter__
  panic("not support in engines other than interpreter");
#endif

  int is_repnz = (s->isa.rep_flags == PREFIX_REPNZ);
  word_t count = (s->isa.rep_flags ? cpu.ecx : 1);
  if (count != 0) {
    rtl_lm(s, s0, &cpu.edi, 0, id_dest->width);
    rtl_lm(s, s1, &cpu.esi, 0, id_dest->width);
    rtl_setrelop(s, RELOP_EQ, s2, s0, s1);
    rtl_set_ZF(s, s2);
    rtl_addi(s, &cpu.esi, &cpu.esi, (cpu.DF ? -1 : 1) * id_dest->width);
    rtl_addi(s, &cpu.edi, &cpu.edi, (cpu.DF ? -1 : 1) * id_dest->width);
  }
  if (s->isa.rep_flags && count != 0) {
    cpu.ecx --;
    if ((count - 1 != 0) && (is_repnz ^ cpu.ZF)) rtl_j(s, cpu.pc);
    else {
      rtl_sub(s, s2, s1, s0);
      rtl_update_ZFSF(s, s2, id_dest->width);
      rtl_is_sub_carry(s, s2, s1, s0);
      rtl_set_CF(s, s2);
      rtl_sub(s, s2, s1, s0);
      rtl_is_sub_overflow(s, s2, s2, s1, s0, id_dest->width);
      rtl_set_OF(s, s2);
    }
  }

  print_asm("cmps (%%edi), (%%esi)");
}
