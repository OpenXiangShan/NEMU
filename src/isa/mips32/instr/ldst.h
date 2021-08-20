#ifdef __ICS_EXPORT
def_EHelper(lw) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 4);
}

def_EHelper(sw) {
  rtl_sm(s, ddest, dsrc1, id_src2->imm, 4);
}
#else
#define def_ldst_template(name, rtl_instr, width, mmu_mode) \
  def_EHelper(name) { \
    concat(rtl_, rtl_instr) (s, ddest, dsrc1, id_src2->imm, width, mmu_mode); \
  }

#define def_all_ldst(suffix, mmu_mode) \
  def_ldst_template(concat(lw , suffix), lm , 4, mmu_mode) \
  def_ldst_template(concat(lh , suffix), lms, 2, mmu_mode) \
  def_ldst_template(concat(lb , suffix), lms, 1, mmu_mode) \
  def_ldst_template(concat(lhu, suffix), lm , 2, mmu_mode) \
  def_ldst_template(concat(lbu, suffix), lm , 1, mmu_mode) \
  def_ldst_template(concat(sw , suffix), sm , 4, mmu_mode) \
  def_ldst_template(concat(sh , suffix), sm , 2, mmu_mode) \
  def_ldst_template(concat(sb , suffix), sm , 1, mmu_mode)

def_all_ldst(, MMU_DYNAMIC)
//def_all_ldst(_mmu, MMU_TRANSLATE)

def_EHelper(swl) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);

  // mem.shamt2
  rtl_andi(s, s1, s0, 0x3);
  rtl_slli(s, s1, s1, 3);

  // load the aligned memory word
  rtl_andi(s, s0, s0, ~0x3u);
  rtl_lm(s, s0, s0, 0, 4, MMU_DYNAMIC);

  // prepare memory data
  rtl_srli(s, s0, s0, 8);   // shift 8 bit
  rtl_srl(s, s0, s0, s1);   // second shift
  rtl_sll(s, s0, s0, s1);   // shift back
  rtl_slli(s, s0, s0, 8);   // shift 8 bit

  // reg.shmat = 24 - mem.shmat2
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // prepare register data
  rtl_srl(s, s1, ddest, s1);

  // merge the word
  rtl_or(s, s1, s0, s1);

  // write back
  rtl_addi(s, s0, dsrc1, id_src2->imm);
  rtl_andi(s, s0, s0, ~0x3u);
  rtl_sm(s, s1, s0, 0, 4, MMU_DYNAMIC);
}

def_EHelper(swr) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);

  // mem.shmat2
  rtl_andi(s, s1, s0, 0x3);
  rtl_slli(s, s1, s1, 3);
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // load the aligned memory word
  rtl_andi(s, s0, s0, ~0x3u);
  rtl_lm(s, s0, s0, 0, 4, MMU_DYNAMIC);

  // prepare memory data
  rtl_slli(s, s0, s0, 8);   // shift 8 bit
  rtl_sll(s, s0, s0, s1);   // second shift
  rtl_srl(s, s0, s0, s1);   // shift back
  rtl_srli(s, s0, s0, 8);   // shift 8 bit

  // reg.shmat = 24 - mem.shmat2
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // prepare register data
  rtl_sll(s, s1, ddest, s1);

  // merge the word
  rtl_or(s, s1, s0, s1);

  // write back
  rtl_addi(s, s0, dsrc1, id_src2->imm);
  rtl_andi(s, s0, s0, ~0x3u);
  rtl_sm(s, s1, s0, 0, 4, MMU_DYNAMIC);
}

def_EHelper(lwl) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);

  // mem.shmat2
  rtl_andi(s, s1, s0, 0x3);
  rtl_slli(s, s1, s1, 3);
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // load the aligned memory word
  rtl_andi(s, s0, s0, ~0x3u);
  rtl_lm(s, s0, s0, 0, 4, MMU_DYNAMIC);

  // prepare memory data
  rtl_sll(s, s0, s0, s1);

  // reg.shmat = 24 - mem.shmat2
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // prepare register data
  rtl_slli(s, ddest, ddest, 8);   // shift 8 bit
  rtl_sll(s, ddest, ddest, s1);   // second shift
  rtl_srl(s, ddest, ddest, s1);   // shift back
  rtl_srli(s, ddest, ddest, 8);   // shift 8 bit

  // merge the word
  rtl_or(s, ddest, s0, ddest);
}

def_EHelper(lwr) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);

  // mem.shmat2
  rtl_andi(s, s1, s0, 0x3);
  rtl_slli(s, s1, s1, 3);

  // load the aligned memory word
  rtl_andi(s, s0, s0, ~0x3u);
  rtl_lm(s, s0, s0, 0, 4, MMU_DYNAMIC);

  // prepare memory data
  rtl_srl(s, s0, s0, s1);

  // reg.shmat = 24 - mem.shmat2
  rtl_subi(s, s1, s1, 24);
  rtl_neg(s, s1, s1);

  // prepare register data
  rtl_srli(s, ddest, ddest, 8);   // shift 8 bit
  rtl_srl(s, ddest, ddest, s1);   // second shift
  rtl_sll(s, ddest, ddest, s1);   // shift back
  rtl_slli(s, ddest, ddest, 8);   // shift 8 bit

  // merge the word
  rtl_or(s, ddest, s0, ddest);
}
#endif
