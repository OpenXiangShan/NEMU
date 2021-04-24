def_EHelper(mov) {
  rtl_decode_binary(s, false, true);
  rtl_wb(s, dsrc1);
}

def_EHelper(push) {
  rtl_decode_unary(s, true);
  rtl_push(s, ddest);
}

def_EHelper(pop) {
  rtl_decode_unary(s, false);
  rtl_pop(s, ddest);
}

def_EHelper(lea) {
  rtl_decode_binary(s, false, false);
  rtl_addi(s, ddest, &s->isa.mbr, s->isa.moff);
  rtl_wb_r(s, ddest);
}

def_EHelper(movzb) {
  rt_decode(s, id_dest, false, s->isa.width);
  rt_decode(s, id_src1, true, 1);
  rtl_wb_r(s, dsrc1);
}

def_EHelper(movzw) {
  rt_decode(s, id_dest, false, s->isa.width);
  rt_decode(s, id_src1, true, 2);
  rtl_wb_r(s, dsrc1);
}

def_EHelper(movsb) {
  rt_decode(s, id_dest, false, s->isa.width);
  rt_decode(s, id_src1, true, 1);
  rtl_sext(s, ddest, dsrc1, 1);
  rtl_wb_r(s, ddest);
}

def_EHelper(movsw) {
  rt_decode(s, id_dest, false, s->isa.width);
  rt_decode(s, id_src1, true, 2);
  rtl_sext(s, ddest, dsrc1, 2);
  rtl_wb_r(s, ddest);
}

def_EHelper(cwtl) {
  if (s->isa.width == 2) {
    rtl_sext(s, s0, &cpu.eax, 1);
    rtl_sr(s, R_AX, s0, 2);
  }
  else {
    rtl_sext(s, &cpu.eax, &cpu.eax, 2);
  }
}

def_EHelper(cltd) {
  if (s->isa.width == 2) { TODO(); }
  else { rtl_sari(s, &cpu.edx, &cpu.eax, 31); }
}

def_EHelper(leave) {
  rtl_mv(s, &cpu.esp, &cpu.ebp);
  rtl_pop(s, &cpu.ebp);
}

def_EHelper(pusha) {
  rtl_mv(s, s0, &cpu.esp);
  rtl_push(s, &cpu.eax);
  rtl_push(s, &cpu.ecx);
  rtl_push(s, &cpu.edx);
  rtl_push(s, &cpu.ebx);
  rtl_push(s, s0);
  rtl_push(s, &cpu.ebp);
  rtl_push(s, &cpu.esi);
  rtl_push(s, &cpu.edi);
}

def_EHelper(popa) {
  rtl_pop(s, &cpu.edi);
  rtl_pop(s, &cpu.esi);
  rtl_pop(s, &cpu.ebp);
  rtl_pop(s, s0);
  rtl_pop(s, &cpu.ebx);
  rtl_pop(s, &cpu.edx);
  rtl_pop(s, &cpu.ecx);
  rtl_pop(s, &cpu.eax);
}

def_EHelper(xchg) {
  rtl_decode_binary(s, true, true);
  if (ddest != dsrc1) {
    rtl_mv(s, s0, dsrc1);
    if      (id_src1->type == OP_TYPE_REG) rtl_sr(s, id_src1->reg, ddest, s->isa.width);
    else if (id_src1->type == OP_TYPE_MEM) rtl_sm(s, ddest, &s->isa.mbr, s->isa.moff, s->isa.width, MMU_DYNAMIC);
    rtl_wb(s, s0);
  }
}

def_EHelper(cmovcc) {
  rtl_decode_binary(s, false, true);

  uint32_t cc = s->isa.opcode & 0xf;
#ifdef CONFIG_x86_CC_LAZY
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
  rtl_wb(s, ddest);
}

def_EHelper(cmpxchg) {
#ifndef CONFIG_ENGINE_INTERPRETER
  panic("not support in engines other than interpreter");
#endif

  assert(s->isa.width == 4);
  rtl_decode_binary(s, true, true);

  rtl_setrelop(s, RELOP_EQ, s0, &cpu.eax, ddest);
  rtl_set_ZF(s, s0);
  if (cpu.ZF) {
    rtl_wb(s, dsrc1);
  } else {
    rtl_sr(s, R_EAX, ddest, s->isa.width);
  }
}

#if 0
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
#endif
