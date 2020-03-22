static inline make_EHelper(add) {
  rtl_add(s, ddest, dsrc1, dsrc2);
  print_asm_template3(add);
}

static inline make_EHelper(sub) {
  rtl_sub(s, ddest, dsrc1, dsrc2);
  print_asm_template3(sub);
}

static inline make_EHelper(sll) {
  rtl_shl(s, ddest, dsrc1, dsrc2);
  print_asm_template3(sll);
}

static inline make_EHelper(srl) {
  if (s->isa.instr.r.funct7 == 32) {
    // sra
    rtl_sar(s, ddest, dsrc1, dsrc2);
    print_asm_template3(sra);
  }
  else {
    rtl_shr(s, ddest, dsrc1, dsrc2);
    print_asm_template3(srl);
  }
}

static inline make_EHelper(sra) {
  exec_srl(s);
}

static inline make_EHelper(slt) {
  rtl_setrelop(s, RELOP_LT, ddest, dsrc1, dsrc2);
  print_asm_template3(slt);
}

static inline make_EHelper(sltu) {
  rtl_setrelop(s, RELOP_LTU, ddest, dsrc1, dsrc2);
  print_asm_template3(sltu);
}

static inline make_EHelper(xor) {
  rtl_xor(s, ddest, dsrc1, dsrc2);
  print_asm_template3(xor);
}

static inline make_EHelper(or) {
  rtl_or(s, ddest, dsrc1, dsrc2);
  print_asm_template3(or);
}

static inline make_EHelper(and) {
  rtl_and(s, ddest, dsrc1, dsrc2);
  print_asm_template3(and);
}

static inline make_EHelper(addi) {
  rtl_addi(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(addi);
}

static inline make_EHelper(slli) {
  rtl_shli(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(slli);
}

static inline make_EHelper(srli) {
  if (s->isa.instr.r.funct7 == 32) {
    // sra
    rtl_sari(s, ddest, dsrc1, id_src2->imm);
    print_asm_template3(srai);
  }
  else {
    rtl_shri(s, ddest, dsrc1, id_src2->imm);
    print_asm_template3(srli);
  }
}

static inline make_EHelper(srai) {
  exec_srli(s);
}

static inline make_EHelper(slti) {
  rtl_setrelopi(s, RELOP_LT, ddest, dsrc1, id_src2->imm);
  print_asm_template3(slti);
}

static inline make_EHelper(sltui) {
  rtl_setrelopi(s, RELOP_LTU, ddest, dsrc1, id_src2->imm);
  print_asm_template3(sltui);
}

static inline make_EHelper(xori) {
  rtl_xori(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(xori);
}

static inline make_EHelper(ori) {
  rtl_ori(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(ori);
}

static inline make_EHelper(andi) {
  rtl_andi(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(andi);
}

static inline make_EHelper(auipc) {
  rtl_li(s, ddest, id_src1->imm + cpu.pc);
  print_asm_template2(auipc);
}

static inline make_EHelper(lui) {
  rtl_li(s, ddest, id_src1->imm);
  print_asm_template2(lui);
}
