static inline make_EHelper(add) {
  rtl_add(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(add);
}

static inline make_EHelper(sub) {
  rtl_sub(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(sub);
}

static inline make_EHelper(sll) {
  rtl_andi(s, dsrc2, dsrc2, 0x1f);
  rtl_shl(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(sll);
}

static inline make_EHelper(srl) {
  rtl_andi(s, dsrc2, dsrc2, 0x1f);

  if (s->isa.instr.r.funct7 == 32) {
    // sra
    rtl_sar(s, s0, dsrc1, dsrc2);
    print_asm_template3(sra);
  }
  else {
    rtl_shr(s, s0, dsrc1, dsrc2);
    print_asm_template3(srl);
  }

  rtl_sr(s, id_dest->reg, s0, 4);
}

static inline make_EHelper(sra) {
  exec_srl(s);
}

static inline make_EHelper(slt) {
  rtl_setrelop(s, RELOP_LT, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(slt);
}

static inline make_EHelper(sltu) {
  rtl_setrelop(s, RELOP_LTU, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(sltu);
}

static inline make_EHelper(xor) {
  rtl_xor(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(xor);
}

static inline make_EHelper(or) {
  rtl_or(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(or);
}

static inline make_EHelper(and) {
  rtl_and(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(and);
}

static inline make_EHelper(auipc) {
  rtl_add(s, s0, dsrc1, &cpu.pc);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template2(auipc);
}

static inline make_EHelper(lui) {
  rtl_sr(s, id_dest->reg, dsrc1, 4);

  print_asm_template2(lui);
}
