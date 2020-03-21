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
  rtl_shl(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(sll);
}

static inline make_EHelper(srl) {
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

static inline make_EHelper(addi) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(addi);
}

static inline make_EHelper(slli) {
  rtl_shli(s, s0, dsrc1, id_src2->imm);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(slli);
}

static inline make_EHelper(srli) {
  if (s->isa.instr.r.funct7 == 32) {
    // sra
    rtl_sari(s, s0, dsrc1, id_src2->imm);
    print_asm_template3(srai);
  }
  else {
    rtl_shri(s, s0, dsrc1, id_src2->imm);
    print_asm_template3(srli);
  }

  rtl_sr(s, id_dest->reg, s0, 4);
}

static inline make_EHelper(srai) {
  exec_srli(s);
}

static inline make_EHelper(slti) {
  rtl_setrelopi(s, RELOP_LT, s0, dsrc1, id_src2->imm);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(slti);
}

static inline make_EHelper(sltui) {
  rtl_setrelopi(s, RELOP_LTU, s0, dsrc1, id_src2->imm);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(sltui);
}

static inline make_EHelper(xori) {
  rtl_xori(s, s0, dsrc1, id_src2->imm);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(xori);
}

static inline make_EHelper(ori) {
  rtl_ori(s, s0, dsrc1, id_src2->imm);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(ori);
}

static inline make_EHelper(andi) {
  rtl_andi(s, s0, dsrc1, id_src2->imm);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(andi);
}

static inline make_EHelper(auipc) {
  rtl_li(s, s0, id_src1->imm + cpu.pc);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template2(auipc);
}

static inline make_EHelper(lui) {
  rtl_li(s, s0, id_src1->imm);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template2(lui);
}
