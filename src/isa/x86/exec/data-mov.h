static inline make_EHelper(mov) {
  operand_write(s, id_dest, dsrc1);
  print_asm_template2(mov);
}

static inline make_EHelper(push) {
//  TODO();
  rtl_push(s, ddest);

  print_asm_template1(push);
}

static inline make_EHelper(pop) {
//  TODO();
  rtl_pop(s, ddest);
  operand_write(s, id_dest, ddest);

  print_asm_template1(pop);
}

static inline make_EHelper(pusha) {
  //TODO();

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

static inline make_EHelper(popa) {
  //TODO();

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

static inline make_EHelper(leave) {
//  TODO();
  rtl_mv(s, &cpu.esp, &cpu.ebp);
  rtl_pop(s, &cpu.ebp);

  print_asm("leave");
}

static inline make_EHelper(cltd) {
  if (s->isa.is_operand_size_16) {
    TODO();
  }
  else {
    rtl_sari(s, &cpu.edx, &cpu.eax, 31);
  }

  print_asm(s->isa.is_operand_size_16 ? "cwtl" : "cltd");
}

static inline make_EHelper(cwtl) {
  if (s->isa.is_operand_size_16) {
    //TODO();
    rtl_sext(s, s0, &cpu.eax, 1);
    rtl_sr(s, R_AX, s0, 2);
  }
  else {
    //TODO();
    rtl_sext(s, &cpu.eax, &cpu.eax, 2);
  }

  print_asm(s->isa.is_operand_size_16 ? "cbtw" : "cwtl");
}

static inline make_EHelper(movsx) {
  id_dest->width = s->isa.is_operand_size_16 ? 2 : 4;
  rtl_sext(s, ddest, dsrc1, id_src1->width);
  operand_write(s, id_dest, ddest);
  print_asm_template2(movsx);
}

static inline make_EHelper(movzx) {
  id_dest->width = s->isa.is_operand_size_16 ? 2 : 4;
  operand_write(s, id_dest, dsrc1);
  print_asm_template2(movzx);
}

static inline make_EHelper(lea) {
  rtl_addi(s, ddest, s->isa.mbase, s->isa.moff);
  operand_write(s, id_dest, ddest);
  print_asm_template2(lea);
}

static inline make_EHelper(xchg) {
  if (ddest != dsrc1) {
    rtl_mv(s, s0, dsrc1);
    operand_write(s, id_src1, ddest);
    operand_write(s, id_dest, s0);
  }
  print_asm_template2(xchg);
}
