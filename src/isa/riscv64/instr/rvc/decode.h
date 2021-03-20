#define creg2reg(creg) (creg + 8)

// rotate without shift, e.g. ror(0bxxxxyy, 6, 2) = 0byyxxxx00
static inline uint32_t ror_imm(uint32_t imm, int len, int r) {
  if (r == 0) return imm;
  uint32_t copy = imm | (imm << len);
  uint32_t mask = BITMASK(len) << r;
  uint32_t res = copy & mask;
  return res;
}

static inline void decode_op_C_imm6(Decode *s, uint32_t imm6, bool sign, int shift, int rotate) {
  uint64_t imm = (sign ? SEXT(imm6, 6) << shift : ror_imm(imm6, 6, rotate));
  decode_op_i(s, id_src2, imm, false);
}

static inline void decode_op_C_rd_rs1(Decode *s, bool creg) {
  uint32_t instr = s->isa.instr.val;
  uint32_t rd_rs1 = (creg ? creg2reg(BITS(instr, 9, 7)) : BITS(instr, 11, 7));
  decode_op_r(s, id_src1, rd_rs1, true);
  decode_op_r(s, id_dest, rd_rs1, false);
}

// creg = false for CI, true for CB_shift and CB_andi
static inline void decode_op_rd_rs1_imm6(Decode *s, bool sign, int shift, int rotate, bool creg) {
  uint32_t instr = s->isa.instr.val;
  uint32_t imm6 = (BITS(instr, 12, 12) << 5) | BITS(instr, 6, 2);
  decode_op_C_imm6(s, imm6, sign, shift, rotate);
  decode_op_C_rd_rs1(s, creg);
}

#if 0
// ---------- CR ---------- 

static inline def_DHelper(CR) {
  decode_op_C_rd_rs1(s, false);
  uint32_t rs2 = BITS(s->isa.instr.val, 6, 2);
  decode_op_r(s, id_src2, rs2, true);
}
#endif

// ---------- CI ---------- 

static inline def_DHelper(CI_simm) {
  decode_op_rd_rs1_imm6(s, true, 0, 0, false);
}

static inline def_DHelper(CI_simm_lui) {
  decode_CI_simm(s);
  // the immediate of LUI is placed at id_src1->imm
  id_src1->imm = id_src2->imm << 12;
}

// for shift
static inline def_DHelper(CI_uimm) {
  decode_op_rd_rs1_imm6(s, false, 0, 0, false);
}

static inline def_DHelper(C_LI) {
  decode_CI_simm(s);
  decode_op_r(s, id_src1, 0, true);
}

static inline def_DHelper(C_ADDI16SP) {
  decode_op_r(s, id_src1, 2, true);
  uint32_t instr = s->isa.instr.val;
  sword_t simm = (SEXT(BITS(instr, 12, 12), 1) << 9) | (BITS(instr, 4, 3) << 7) |
    (BITS(instr, 5, 5) << 6) | (BITS(instr, 2, 2) << 5) | (BITS(instr, 6, 6) << 4);
  assert(simm != 0);
  decode_op_i(s, id_src2, simm, false);
  decode_op_r(s, id_dest, 2, false);
}

// SP-relative load/store
static inline void decode_C_xxSP(Decode *s, uint32_t imm6, int rotate) {
  decode_op_r(s, id_src1, 2, true);
  decode_op_C_imm6(s, imm6, false, 0, rotate);
}

static inline void decode_C_LxSP(Decode *s, int rotate, bool is_fp) {
  uint32_t imm6 = (BITS(s->isa.instr.val, 12, 12) << 5) | BITS(s->isa.instr.val, 6, 2);
  decode_C_xxSP(s, imm6, rotate);
  uint32_t rd = BITS(s->isa.instr.val, 11, 7);
  if (is_fp) decode_op_fpr(s, id_dest, rd, false);
  else decode_op_r(s, id_dest, rd, false);
}

static inline def_DHelper(C_LWSP) {
  decode_C_LxSP(s, 2, false);
}

static inline def_DHelper(C_LDSP) {
  decode_C_LxSP(s, 3, false);
}

static inline def_DHelper(C_FLDSP) {
  decode_C_LxSP(s, 3, true);
}

// ---------- CSS ---------- 

static inline void decode_C_SxSP(Decode *s, int rotate, bool is_fp) {
  uint32_t imm6 = BITS(s->isa.instr.val, 12, 7);
  decode_C_xxSP(s, imm6, rotate);
  uint32_t rs2 = BITS(s->isa.instr.val, 6, 2);
  if (is_fp) decode_op_fpr(s, id_dest, rs2, true);
  else decode_op_r(s, id_dest, rs2, true);
}

static inline def_DHelper(C_SWSP) {
  decode_C_SxSP(s, 2, false);
}

static inline def_DHelper(C_SDSP) {
  decode_C_SxSP(s, 3, false);
}

static inline def_DHelper(C_FSDSP) {
  decode_C_SxSP(s, 3, true);
}

// ---------- CIW ---------- 

static inline def_DHelper(C_ADDI4SPN) {
  decode_op_r(s, id_src1, 2, true);
  uint32_t instr = s->isa.instr.val;
  uint32_t imm9_6 = ror_imm(BITS(instr, 12, 7), 6, 4); // already at the right place
  uint32_t imm = imm9_6 | BITS(instr, 5, 5) << 3 | BITS(instr, 6, 6) << 2;
  Assert(imm != 0, "pc = " FMT_WORD, s->pc);
  decode_op_i(s, id_src2, imm, false);
  decode_op_r(s, id_dest, creg2reg(BITS(instr, 4, 2)), false);
}

// ---------- CL ---------- 

// load/store
static inline void decode_C_ldst_common(Decode *s, int rotate, bool is_store, bool is_fp) {
  uint32_t instr = s->isa.instr.val;
  decode_op_r(s, id_src1, creg2reg(BITS(instr, 9, 7)), true);
  uint32_t imm5 = (BITS(instr, 12, 10) << 2) | BITS(instr, 6, 5);
  uint32_t imm = ror_imm(imm5, 5, rotate) << 1;
  decode_op_i(s, id_src2, imm, false);
  if (is_fp) decode_op_fpr(s, id_dest, creg2reg(BITS(instr, 4, 2)), is_store);
  else decode_op_r(s, id_dest, creg2reg(BITS(instr, 4, 2)), is_store);
}

static inline def_DHelper(C_LW) {
  decode_C_ldst_common(s, 1, false, false);
}

static inline def_DHelper(C_LD) {
  decode_C_ldst_common(s, 2, false, false);
}

static inline def_DHelper(C_FLD) {
  decode_C_ldst_common(s, 2, false, true);
}

// ---------- CS ---------- 

static inline def_DHelper(C_SW) {
  decode_C_ldst_common(s, 1, true, false);
}

static inline def_DHelper(C_SD) {
  decode_C_ldst_common(s, 2, true, false);
}

static inline def_DHelper(C_FSD) {
  decode_C_ldst_common(s, 2, true, true);
}

static inline def_DHelper(CS) {
  decode_op_C_rd_rs1(s, true);
  uint32_t rs2 = creg2reg(BITS(s->isa.instr.val, 4, 2));
  decode_op_r(s, id_src2, rs2, true);
}

// ---------- CB ---------- 

static inline def_DHelper(CB) {
  uint32_t instr = s->isa.instr.val;
  sword_t simm8 = SEXT(BITS(instr, 12, 12), 1);
  uint32_t imm7_6 = BITS(instr, 6, 5);
  uint32_t imm5 = BITS(instr, 2, 2);
  uint32_t imm4_3 = BITS(instr, 11, 10);
  uint32_t imm2_1 = BITS(instr, 4, 3);
  sword_t offset = (simm8 << 8) | (imm7_6 << 6) | (imm5 << 5) | (imm4_3 << 3) | (imm2_1 << 1);
  decode_op_i(s, id_dest, s->pc + offset, true);
  decode_op_r(s, id_src1, creg2reg(BITS(instr, 9, 7)), true);
  decode_op_r(s, id_src2, 0, true);
}

static inline def_DHelper(CB_shift) {
  decode_op_rd_rs1_imm6(s, false, 0, 0, true);
}

static inline def_DHelper(CB_andi) {
  decode_op_rd_rs1_imm6(s, true, 0, 0, true);
}

// ---------- CJ ---------- 

static inline def_DHelper(CJ) {
  uint32_t instr = s->isa.instr.val;
  sword_t simm11 = SEXT(BITS(instr, 12, 12), 1);
  uint32_t imm10  = BITS(instr, 8, 8);
  uint32_t imm9_8 = BITS(instr, 10, 9);
  uint32_t imm7   = BITS(instr, 6, 6);
  uint32_t imm6   = BITS(instr, 7, 7);
  uint32_t imm5   = BITS(instr, 2, 2);
  uint32_t imm4   = BITS(instr, 11, 11);
  uint32_t imm3_1 = BITS(instr, 5, 3);

  sword_t offset = (simm11 << 11) | (imm10 << 10) | (imm9_8 << 8) |
    (imm7 << 7) | (imm6 << 6) | (imm5 << 5) | (imm4 << 4) | (imm3_1 << 1);
  decode_op_i(s, id_src1, s->pc + offset, true);
}

static inline void decode_C_rs1_rs2_rd(Decode *s, bool is_rs1_zero, bool is_rs2_zero, bool is_rd_zero) {
  decode_op_r(s, id_src1, (is_rs1_zero ? 0 : BITS(s->isa.instr.val, 11, 7)), true);
  decode_op_r(s, id_src2, (is_rs2_zero ? 0 : BITS(s->isa.instr.val, 6, 2)), true);
  decode_op_r(s, id_dest, (is_rd_zero ? 0 : BITS(s->isa.instr.val, 11, 7)), false);
}

static inline def_DHelper(C_JR) {
  decode_op_r(s, id_src1, BITS(s->isa.instr.val, 11, 7), true);
}

static inline def_DHelper(C_MOV) {
  // we should put the source at src1 to call c_mv()
  decode_op_r(s, id_src1, BITS(s->isa.instr.val, 6, 2), true);
  decode_op_r(s, id_src2, 0, true);
  decode_op_r(s, id_dest, BITS(s->isa.instr.val, 11, 7), false);
}

static inline def_DHelper(C_ADD) {
  decode_C_rs1_rs2_rd(s, false, false, false);
}

static inline def_DHelper(C_JALR) {
  decode_op_r(s, id_src1, BITS(s->isa.instr.val, 11, 7), true);
  decode_op_i(s, id_src2, 0, true);
  decode_op_r(s, id_dest, 1, false);
}


def_THelper(c_addi_dispatch) {
  if (id_src2->imm == 1) return table_p_inc(s);
  if (id_src2->imm == -1ul) return table_p_dec(s);
  return table_c_addi(s);
}

def_THelper(op_imm_c) {
  if ((s->isa.instr.r.funct7 & ~0x1) == 32) {
    switch (s->isa.instr.r.funct3) { TAB(5, c_srai) }
  }
  switch (s->isa.instr.i.funct3) {
    TAB(0, c_addi_dispatch)  TAB(1, c_slli)  TAB(2, slti) TAB(3, sltui)
    TAB(4, xori)  TAB(5, c_srli)  TAB(6, ori)  TAB(7, c_andi)
  }
  return EXEC_ID_inv;
};


def_THelper(c_addiw_dispatch) {
  return table_c_addiw(s);
}

def_THelper(op_imm32_c) {
  if (s->isa.instr.r.funct7 == 32) {
    switch (s->isa.instr.r.funct3) { TAB(5, sraiw) }
  }
  switch (s->isa.instr.i.funct3) {
    TAB(0, c_addiw_dispatch) TAB(1, slliw) TAB(5, srliw)
  }
  return EXEC_ID_inv;
}

def_THelper(c_jr_dispatch) {
  if (BITS(s->isa.instr.val, 11, 7) == 1) return table_p_ret(s);
  return table_c_jr(s);
}

def_THelper(c_jalr_dispatch) {
  if (BITS(s->isa.instr.val, 11, 7) != 1) return table_c_jalr(s);
  return table_jalr(s); // c_jalr can not handle correctly when rs1 == ra
}

def_THelper(misc) {
  uint32_t instr = s->isa.instr.val;
  uint32_t bits12not0 = (BITS(instr, 12, 12) != 0);
  uint32_t bits11_7not0 = (BITS(instr, 11, 7) != 0);
  uint32_t bits6_2not0 = (BITS(instr, 6, 2) != 0);
  uint32_t op = (bits12not0 << 2) | (bits11_7not0 << 1) | bits6_2not0;
  switch (op) {
    IDTAB(0b010, C_JR, c_jr_dispatch)
    IDTAB(0b011, C_MOV, c_mv)
    IDTAB(0b110, C_JALR, c_jalr_dispatch)
    IDTAB(0b111, C_ADD, c_add)
  }
  return EXEC_ID_inv;
}

def_THelper(lui_addi16sp) {
  uint32_t rd = BITS(s->isa.instr.val, 11, 7);
  assert(rd != 0);
  switch (rd) {
    IDTAB(2, C_ADDI16SP, c_addi)
    default: // and other cases
    IDTAB(1, CI_simm_lui, lui)
  }
  return EXEC_ID_inv;
}

def_THelper(misc_alu) {
  uint32_t instr = s->isa.instr.val;
  uint32_t op = BITS(instr, 11, 10);
  if (op == 3) {
    uint32_t op2 = (BITS(instr, 12, 12) << 2) | BITS(instr, 6, 5);
    switch (op2) {
      IDTAB(0, CS, c_sub) IDTAB(1, CS, c_xor) IDTAB(2, CS, c_or)  IDTAB(3, CS, c_and)
      IDTAB(4, CS, c_subw)IDTAB(5, CS, c_addw)
    }
  } else {
    switch (op) {
      IDTAB(0, CB_shift, c_srli)
      IDTAB(1, CB_shift, c_srai)
      IDTAB(2, CB_andi,  c_andi)
    }
  }
  return EXEC_ID_inv;
}

def_THelper(ld_dispatch) {
  int mmu_mode = isa_mmu_state();
  if (mmu_mode == MMU_DIRECT) return table_ld(s);
  else if (mmu_mode == MMU_TRANSLATE) return table_ld_mmu(s);
  else assert(0);
}

def_THelper(lw_dispatch) {
  int mmu_mode = isa_mmu_state();
  if (mmu_mode == MMU_DIRECT) return table_lw(s);
  else if (mmu_mode == MMU_TRANSLATE) return table_lw_mmu(s);
  else assert(0);
}

def_THelper(sd_dispatch) {
  int mmu_mode = isa_mmu_state();
  if (mmu_mode == MMU_DIRECT) return table_sd(s);
  else if (mmu_mode == MMU_TRANSLATE) return table_sd_mmu(s);
  else assert(0);
}

def_THelper(sw_dispatch) {
  int mmu_mode = isa_mmu_state();
  if (mmu_mode == MMU_DIRECT) return table_sw(s);
  else if (mmu_mode == MMU_TRANSLATE) return table_sw_mmu(s);
  else assert(0);
}

def_THelper(rvc) {
  uint32_t rvc_opcode = (s->isa.instr.r.opcode1_0 << 3) | BITS(s->isa.instr.val, 15, 13);
  switch (rvc_opcode) {
    IDTAB(000, C_ADDI4SPN, addi)          IDTAB(001, C_FLD, fld)
    IDTAB(002, C_LW, lw_dispatch)         IDTAB(003, C_LD, ld_dispatch)
                                          IDTAB(005, C_FSD, fsd)
    IDTAB(006, C_SW, sw_dispatch)         IDTAB(007, C_SD, sd_dispatch)
    IDTAB(010, CI_simm, c_addi_dispatch)  IDTAB(011, CI_simm, c_addiw_dispatch)
    IDTAB(012, C_LI, li_dispatch)         TAB  (013, lui_addi16sp)
    TAB  (014, misc_alu)                  IDTAB(015, CJ, c_j)
    IDTAB(016, CB, c_beqz)                IDTAB(017, CB, c_bnez)
    IDTAB(020, CI_uimm, c_slli)           IDTAB(021, C_FLDSP, fld)
    IDTAB(022, C_LWSP, lw_dispatch)       IDTAB(023, C_LDSP, ld_dispatch)
    TAB  (024, misc)                      IDTAB(025, C_FSDSP, fsd)
    IDTAB(026, C_SWSP, sw_dispatch)       IDTAB(027, C_SDSP, sd_dispatch)
  }
  return table_inv(s);
};

