#define def_EWDWBHelper(name, id, exec, width, wb) \
  def_EHelper(name) { \
    concat(rt_decode_, id) (s, concat(WIDTH_, width)); \
    concat(rt_exec_, exec) (s); \
    concat(rt_wb_, wb) (s, concat(WIDTH_, width)); \
  }

def_EWDWBHelper(movl_I2r, mov_I2r, mov, l, r);

#if 0
def_EHelper(mov) {
  operand_write(s, id_dest, dsrc1);
}

#ifndef __ICS_EXPORT
static inline def_EHelper(push) {
  rtl_push(s, ddest);
  print_asm_template1(push);
}

static inline def_EHelper(pop) {
  rtl_pop(s, ddest);
  operand_write(s, id_dest, ddest);
  print_asm_template1(pop);
}

static inline def_EHelper(pusha) {
  rtl_mv(s, s0, &cpu.esp);
  rtl_push(s, &cpu.eax);
  rtl_push(s, &cpu.ecx);
  rtl_push(s, &cpu.edx);
  rtl_push(s, &cpu.ebx);
  rtl_push(s, s0);
  rtl_push(s, &cpu.ebp);
  rtl_push(s, &cpu.esi);
  rtl_push(s, &cpu.edi);
  print_asm("pusha");
}

static inline def_EHelper(popa) {
  rtl_pop(s, &cpu.edi);
  rtl_pop(s, &cpu.esi);
  rtl_pop(s, &cpu.ebp);
  rtl_pop(s, s0);
  rtl_pop(s, &cpu.ebx);
  rtl_pop(s, &cpu.edx);
  rtl_pop(s, &cpu.ecx);
  rtl_pop(s, &cpu.eax);
  print_asm("popa");
}

static inline def_EHelper(leave) {
  rtl_mv(s, &cpu.esp, &cpu.ebp);
  rtl_pop(s, &cpu.ebp);
  print_asm("leave");
}

static inline def_EHelper(cltd) {
  if (s->isa.is_operand_size_16) {
    TODO();
  }
  else {
    rtl_sari(s, &cpu.edx, &cpu.eax, 31);
  }
  print_asm(s->isa.is_operand_size_16 ? "cwtl" : "cltd");
}

static inline def_EHelper(cwtl) {
  if (s->isa.is_operand_size_16) {
    rtl_sext(s, s0, &cpu.eax, 1);
    rtl_sr(s, R_AX, s0, 2);
  }
  else {
    rtl_sext(s, &cpu.eax, &cpu.eax, 2);
  }

  print_asm(s->isa.is_operand_size_16 ? "cbtw" : "cwtl");
}

static inline def_EHelper(xchg) {
  if (ddest != dsrc1) {
    rtl_mv(s, s0, dsrc1);
    operand_write(s, id_src1, ddest);
    operand_write(s, id_dest, s0);
  }
  print_asm_template2(xchg);
}

static inline def_EHelper(cmpxchg) {
#ifndef CONFIG_ENGINE_INTERPRETER
  panic("not support in engines other than interpreter");
#endif

  rtl_setrelop(s, RELOP_EQ, s0, dsrc1, ddest);
  rtl_set_ZF(s, s0);
  if (cpu.ZF) {
    operand_write(s, id_dest, dsrc2);
  } else {
    operand_write(s, id_src1, ddest);
  }

  print_asm_template2(cmpxchg);
}

static inline def_EHelper(cmpxchg8b) {
#ifndef CONFIG_ENGINE_INTERPRETER
  panic("not support in engines other than interpreter");
#endif

  // first 4 bytes of the memory operand is already loaded by the decode helper
  rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 4, 4);
  rtl_setrelop(s, RELOP_EQ, &id_src1->val, &cpu.eax, ddest);
  rtl_setrelop(s, RELOP_EQ, &id_src2->val, &cpu.edx, s0);
  rtl_and(s, &id_src1->val, &id_src1->val, &id_src2->val);
  rtl_set_ZF(s, &id_src1->val);
  if (cpu.ZF) {
    rtl_sm(s, s->isa.mbase, s->isa.moff + 0, &cpu.ebx, 4);
    rtl_sm(s, s->isa.mbase, s->isa.moff + 4, &cpu.ecx, 4);
  } else {
    rtl_mv(s, &cpu.eax, ddest);
    rtl_mv(s, &cpu.edx, s0);
  }

  print_asm_template2(cmpxchg8b);
}

static inline def_EHelper(cmovcc) {
  uint32_t cc = s->opcode & 0xf;
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, cc);
#else
  rtl_setcc(s, s0, cc);
#endif

  // ddest <- (s0 ? dsrc1 : ddest)
  rtl_setrelopi(s, RELOP_EQ, s0, s0, 0);
  rtl_subi(s, s0, s0, 1);
  // s0 = mask
  rtl_and(s, s1, dsrc1, s0);
  rtl_not(s, s0, s0);
  rtl_and(s, ddest, ddest, s0);
  rtl_or(s, ddest, ddest, s1);

  operand_write(s, id_dest, ddest);

  print_asm("cmov%s %s,%s", get_cc_name(cc), id_src1->str, id_dest->str);
}
#else
static inline def_EHelper(push) {
  TODO();
  print_asm_template1(push);
}

static inline def_EHelper(pop) {
  TODO();
  print_asm_template1(pop);
}

static inline def_EHelper(pusha) {
  TODO();
  print_asm("pusha");
}

static inline def_EHelper(popa) {
  TODO();
  print_asm("popa");
}

static inline def_EHelper(leave) {
  TODO();
  print_asm("leave");
}

static inline def_EHelper(cltd) {
  if (s->isa.is_operand_size_16) {
    TODO();
  }
  else {
    TODO();
  }
  print_asm(s->isa.is_operand_size_16 ? "cwtl" : "cltd");
}

static inline def_EHelper(cwtl) {
  if (s->isa.is_operand_size_16) {
    TODO();
  }
  else {
    TODO();
  }
  print_asm(s->isa.is_operand_size_16 ? "cbtw" : "cwtl");
}
#endif

static inline def_EHelper(movsx) {
  id_dest->width = s->isa.is_operand_size_16 ? 2 : 4;
  rtl_sext(s, ddest, dsrc1, id_src1->width);
  operand_write(s, id_dest, ddest);
  print_asm_template2(movsx);
}

static inline def_EHelper(movzx) {
  id_dest->width = s->isa.is_operand_size_16 ? 2 : 4;
  operand_write(s, id_dest, dsrc1);
  print_asm_template2(movzx);
}

static inline def_EHelper(lea) {
  rtl_addi(s, ddest, s->isa.mbase, s->isa.moff);
  operand_write(s, id_dest, ddest);
  print_asm_template2(lea);
}
#endif
