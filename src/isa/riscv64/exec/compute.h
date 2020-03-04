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
  rtl_andi(s, dsrc2, dsrc2, 0x3f);
  rtl_shl(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(sll);
}

static inline make_EHelper(sra) {
  rtl_andi(s, dsrc2, dsrc2, 0x3f);
  rtl_sar(s, s0, dsrc1, dsrc2);
  print_asm_template3(sra);
  rtl_sr(s, id_dest->reg, s0, 4);
}

static inline make_EHelper(srl) {
  // the LSB of funct7 may be "1" due to the shift amount can be >= 32
  // this rule is disabled when a compressed inst comes in
  if ((s->isa.instr.r.funct7 & ~0x1) == 32 && s->isa.instr.r.opcode1_0 == 0x3) {
    exec_sra(s);
    return;
  }
  rtl_andi(s, dsrc2, dsrc2, 0x3f);
  rtl_shr(s, s0, dsrc1, dsrc2);
  print_asm_template3(srl);
  rtl_sr(s, id_dest->reg, s0, 4);
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
  rtl_shli(s, s0, dsrc2, 12);
  rtl_add(s, s0, s0, &cpu.pc);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template2(auipc);
}

static inline make_EHelper(lui) {
  rtl_shli(s, s0, dsrc2, 12);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template2(lui);
}

static inline make_EHelper(addw) {
  rtl_add(s, s0, dsrc1, dsrc2);
  rtl_sext(s, s0, s0, 4);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(addw);
}

static inline make_EHelper(subw) {
  rtl_sub(s, s0, dsrc1, dsrc2);
  rtl_sext(s, s0, s0, 4);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(subw);
}

static inline make_EHelper(sllw) {
  rtl_andi(s, dsrc2, dsrc2, 0x1f);
  rtl_shl(s, s0, dsrc1, dsrc2);
  rtl_sext(s, s0, s0, 4);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(sllw);
}

static inline make_EHelper(srlw) {
  rtl_andi(s, dsrc2, dsrc2, 0x1f);
  assert((s->isa.instr.r.funct7 & 0x1) == 0);
  if (s->isa.instr.r.funct7 == 32) {
    // sraw
    rtl_sext(s, dsrc1, dsrc1, 4);
    rtl_sar(s, s0, dsrc1, dsrc2);
    rtl_sext(s, s0, s0, 4);
    print_asm_template3(sraw);
  }
  else {
    // srlw
    rtl_andi(s, dsrc1, dsrc1, 0xffffffffu);
    rtl_shr(s, s0, dsrc1, dsrc2);
    rtl_sext(s, s0, s0, 4);
    print_asm_template3(srlw);
  }

  rtl_sr(s, id_dest->reg, s0, 4);
}

static inline make_EHelper(sraw) {
  exec_srlw(s);
}
