def_EHelper(lw) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 4);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lw);
}

def_EHelper(sw) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 4);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(sw);
}
#ifndef __ICS_EXPORT

def_EHelper(lh) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lh);
}

def_EHelper(lb) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lb);
}

def_EHelper(lhu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lhu);
}

def_EHelper(lbu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lbu);
}

def_EHelper(sh) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(sh);
}

def_EHelper(sb) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(sb);
}

def_EHelper(swl) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);

  // mem.shamt2
  rtl_andi(s, s1, s0, 0x3);
  rtl_shli(s, s1, s1, 3);

  // load the aligned memory word
  rtl_andi(s, s0, s0, ~0x3u);
  try { rtl_lm(s, s0, s0, 0, 4); } catch;

  // prepare memory data
  rtl_shri(s, s0, s0, 8);   // shift 8 bit
  rtl_shr(s, s0, s0, s1);   // second shift
  rtl_shl(s, s0, s0, s1);   // shift back
  rtl_shli(s, s0, s0, 8);   // shift 8 bit

  // reg.shmat = 24 - mem.shmat2
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // prepare register data
  rtl_shr(s, s1, ddest, s1);

  // merge the word
  rtl_or(s, s1, s0, s1);

  // write back
  rtl_addi(s, s0, dsrc1, id_src2->imm);
  rtl_andi(s, s0, s0, ~0x3u);
  try { rtl_sm(s, s0, 0, s1, 4); } catch;

  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(swl);
}

def_EHelper(swr) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);

  // mem.shmat2
  rtl_andi(s, s1, s0, 0x3);
  rtl_shli(s, s1, s1, 3);
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // load the aligned memory word
  rtl_andi(s, s0, s0, ~0x3u);
  try { rtl_lm(s, s0, s0, 0, 4); } catch;

  // prepare memory data
  rtl_shli(s, s0, s0, 8);   // shift 8 bit
  rtl_shl(s, s0, s0, s1);   // second shift
  rtl_shr(s, s0, s0, s1);   // shift back
  rtl_shri(s, s0, s0, 8);   // shift 8 bit

  // reg.shmat = 24 - mem.shmat2
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // prepare register data
  rtl_shl(s, s1, ddest, s1);

  // merge the word
  rtl_or(s, s1, s0, s1);

  // write back
  rtl_addi(s, s0, dsrc1, id_src2->imm);
  rtl_andi(s, s0, s0, ~0x3u);
  try { rtl_sm(s, s0, 0, s1, 4); } catch;

  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(swr);
}

def_EHelper(lwl) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);

  // mem.shmat2
  rtl_andi(s, s1, s0, 0x3);
  rtl_shli(s, s1, s1, 3);
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // load the aligned memory word
  rtl_andi(s, s0, s0, ~0x3u);
  try { rtl_lm(s, s0, s0, 0, 4); } catch;

  // prepare memory data
  rtl_shl(s, s0, s0, s1);

  // reg.shmat = 24 - mem.shmat2
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // prepare register data
  rtl_shli(s, ddest, ddest, 8);   // shift 8 bit
  rtl_shl(s, ddest, ddest, s1);   // second shift
  rtl_shr(s, ddest, ddest, s1);   // shift back
  rtl_shri(s, ddest, ddest, 8);   // shift 8 bit

  // merge the word
  rtl_or(s, ddest, s0, ddest);

  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lwl);
}

def_EHelper(lwr) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);

  // mem.shmat2
  rtl_andi(s, s1, s0, 0x3);
  rtl_shli(s, s1, s1, 3);

  // load the aligned memory word
  rtl_andi(s, s0, s0, ~0x3u);
  try { rtl_lm(s, s0, s0, 0, 4); } catch;

  // prepare memory data
  rtl_shr(s, s0, s0, s1);

  // reg.shmat = 24 - mem.shmat2
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // prepare register data
  rtl_shri(s, ddest, ddest, 8);   // shift 8 bit
  rtl_shr(s, ddest, ddest, s1);   // second shift
  rtl_shl(s, ddest, ddest, s1);   // shift back
  rtl_shli(s, ddest, ddest, 8);   // shift 8 bit

  // merge the word
  rtl_or(s, ddest, s0, ddest);

  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lwr);
}
#endif
