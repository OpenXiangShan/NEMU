static inline def_EHelper(add) {
  rtl_add(s, ddest, dsrc1, dsrc2);
  print_asm_template3(add);
}

static inline def_EHelper(sub) {
  rtl_sub(s, ddest, dsrc1, dsrc2);
  print_asm_template3(sub);
}

static inline def_EHelper(sll) {
  rtl_shl(s, ddest, dsrc1, dsrc2);
  print_asm_template3(sll);
}

static inline def_EHelper(sra) {
  rtl_sar(s, ddest, dsrc1, dsrc2);
  print_asm_template3(sra);
}

static inline def_EHelper(srl) {
  // the LSB of funct7 may be "1" due to the shift amount can be >= 32
  // this rule is disabled when a compressed inst comes in
  if ((s->isa.instr.r.funct7 & ~0x1) == 32 && s->isa.instr.r.opcode1_0 == 0x3) {
    exec_sra(s);
    return;
  }
  rtl_shr(s, ddest, dsrc1, dsrc2);
  print_asm_template3(srl);
}

static inline def_EHelper(slt) {
  rtl_setrelop(s, RELOP_LT, ddest, dsrc1, dsrc2);
  print_asm_template3(slt);
}

static inline def_EHelper(sltu) {
  rtl_setrelop(s, RELOP_LTU, ddest, dsrc1, dsrc2);
  print_asm_template3(sltu);
}

static inline def_EHelper(xor) {
  rtl_xor(s, ddest, dsrc1, dsrc2);
  print_asm_template3(xor);
}

static inline def_EHelper(or) {
  rtl_or(s, ddest, dsrc1, dsrc2);
  print_asm_template3(or);
}

static inline def_EHelper(and) {
  rtl_and(s, ddest, dsrc1, dsrc2);
  print_asm_template3(and);
}

static inline def_EHelper(addi) {
  rtl_addi(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(addi);
}

static inline def_EHelper(slli) {
  rtl_shli(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(slli);
}

static inline def_EHelper(srai) {
  rtl_sari(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(srai);
}

static inline def_EHelper(srli) {
  // the LSB of funct7 may be "1" due to the shift amount can be >= 32
  // this rule is disabled when a compressed inst comes in
  if ((s->isa.instr.r.funct7 & ~0x1) == 32 && s->isa.instr.r.opcode1_0 == 0x3) {
    exec_srai(s);
  } else {
    rtl_shri(s, ddest, dsrc1, id_src2->imm);
    print_asm_template3(srli);
  }
}

static inline def_EHelper(slti) {
  rtl_setrelopi(s, RELOP_LT, ddest, dsrc1, id_src2->imm);
  print_asm_template3(slti);
}

static inline def_EHelper(sltui) {
  rtl_setrelopi(s, RELOP_LTU, ddest, dsrc1, id_src2->imm);
  print_asm_template3(sltui);
}

static inline def_EHelper(xori) {
  rtl_xori(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(xori);
}

static inline def_EHelper(ori) {
  rtl_ori(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(ori);
}

static inline def_EHelper(andi) {
  rtl_andi(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(andi);
}

static inline def_EHelper(auipc) {
  rtl_li(s, ddest, id_src2->imm + cpu.pc);
  print_asm("auipc " FMT_WORD ",%s", *s0, id_dest->str);
}

static inline def_EHelper(lui) {
  rtl_li(s, ddest, id_src2->imm);
  print_asm("lui " FMT_WORD ",%s", *s0, id_dest->str);
}

static inline def_EHelper(addw) {
  rtl_addw(s, ddest, dsrc1, dsrc2);
  print_asm_template3(addw);
}

static inline def_EHelper(subw) {
  rtl_subw(s, ddest, dsrc1, dsrc2);
  print_asm_template3(subw);
}

static inline def_EHelper(sllw) {
  rtl_shlw(s, ddest, dsrc1, dsrc2);
  print_asm_template3(sllw);
}

static inline def_EHelper(srlw) {
  assert((s->isa.instr.r.funct7 & 0x1) == 0);
  if (s->isa.instr.r.funct7 == 32) {
    rtl_sarw(s, ddest, dsrc1, dsrc2);
    print_asm_template3(sraw);
  }
  else {
    rtl_shrw(s, ddest, dsrc1, dsrc2);
    print_asm_template3(srlw);
  }
}

static inline def_EHelper(sraw) {
  exec_srlw(s);
}

static inline def_EHelper(addiw) {
  rtl_addiw(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(addiw);
}

static inline def_EHelper(slliw) {
  rtl_shliw(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(slliw);
}

static inline def_EHelper(srliw) {
  assert((s->isa.instr.r.funct7 & 0x1) == 0);
  if (s->isa.instr.r.funct7 == 32) {
    rtl_sariw(s, ddest, dsrc1, id_src2->imm);
    print_asm_template3(sraiw);
  }
  else {
    rtl_shriw(s, ddest, dsrc1, id_src2->imm);
    print_asm_template3(srliw);
  }
}
