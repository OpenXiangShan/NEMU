#include "../local-include/rtl.h"
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <isa-all-instr.h>

def_all_THelper();

// decode operand helper
#define def_DopHelper(name) \
  void concat(decode_op_, name) (Decode *s, Operand *op, uint64_t val, bool flag)

static inline def_DopHelper(i) {
  op->imm = val;
  print_Dop(op->str, OP_STR_SIZE, (flag ? "0x%lx" : "%ld"), op->imm);
}

static inline def_DopHelper(r) {
  bool load_val = flag;
  static word_t zero_null = 0;
  op->preg = (!load_val && val == 0) ? &zero_null : &reg_l(val);
  IFDEF(CONFIG_DEBUG, op->reg = val);
  print_Dop(op->str, OP_STR_SIZE, "%s", reg_name(val, 4));
}

#if 0
static inline def_DopHelper(fpr){
  op->type = OP_TYPE_REG;
  op->reg = val;
  op->preg = &fpreg_l(val);

  print_Dop(op->str, OP_STR_SIZE, "%s", fpreg_name(op->reg, 4));
}
#endif

static inline def_DHelper(I) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, (sword_t)s->isa.instr.i.simm11_0, false);
  decode_op_r(s, id_dest, s->isa.instr.i.rd, false);
}

static inline def_DHelper(R) {
  decode_op_r(s, id_src1, s->isa.instr.r.rs1, true);
  decode_op_r(s, id_src2, s->isa.instr.r.rs2, true);
  decode_op_r(s, id_dest, s->isa.instr.r.rd, false);
}

static inline def_DHelper(U) {
  decode_op_i(s, id_src1, (sword_t)s->isa.instr.u.simm31_12 << 12, true);
  decode_op_r(s, id_dest, s->isa.instr.u.rd, false);
}

static inline def_DHelper(auipc) {
  decode_U(s);
  id_src1->imm += s->pc;
}

static inline def_DHelper(J) {
  sword_t offset = (s->isa.instr.j.simm20 << 20) | (s->isa.instr.j.imm19_12 << 12) |
    (s->isa.instr.j.imm11 << 11) | (s->isa.instr.j.imm10_1 << 1);
  decode_op_i(s, id_src1, s->pc + offset, true);
  decode_op_r(s, id_dest, s->isa.instr.j.rd, false);
  id_src2->imm = s->snpc;
}

static inline def_DHelper(B) {
  sword_t offset = (s->isa.instr.b.simm12 << 12) | (s->isa.instr.b.imm11 << 11) |
    (s->isa.instr.b.imm10_5 << 5) | (s->isa.instr.b.imm4_1 << 1);
  decode_op_i(s, id_dest, s->pc + offset, true);
  decode_op_r(s, id_src1, s->isa.instr.b.rs1, true);
  decode_op_r(s, id_src2, s->isa.instr.b.rs2, true);
}

static inline def_DHelper(S) {
  decode_op_r(s, id_src1, s->isa.instr.s.rs1, true);
  sword_t simm = (s->isa.instr.s.simm11_5 << 5) | s->isa.instr.s.imm4_0;
  decode_op_i(s, id_src2, simm, false);
  decode_op_r(s, id_dest, s->isa.instr.s.rs2, true);
}

static inline def_DHelper(csr) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.csr.csr, true);
  decode_op_r(s, id_dest, s->isa.instr.i.rd, false);
}

static inline def_DHelper(csri) {
  decode_op_i(s, id_src1, s->isa.instr.i.rs1, false);
  decode_op_i(s, id_src2, s->isa.instr.csr.csr, true);
  decode_op_r(s, id_dest, s->isa.instr.i.rd, false);
}

#if 0
// RVF RVD

static inline void decode_fp_width(Decode *s) {
  switch (s->isa.instr.fp.fmt)
  {
  case 0: // RVF
    s->width = 4;
    break;
  case 1: // RVD
    s->width = 8;
    break;
  default:
    assert(0);
    break;
  }
}

// --------- fpr to fpr ---------

static inline def_DHelper(F_R) {
  decode_op_fpr(s, id_src1, s->isa.instr.fp.rs1, true);
  decode_op_fpr(s, id_src2, s->isa.instr.fp.rs2, true);
  decode_op_fpr(s, id_dest, s->isa.instr.fp.rd, false);
  decode_fp_width(s);
}

// --------- FLD/FLW --------- 

static inline def_DHelper(F_I) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, (sword_t)s->isa.instr.i.simm11_0, false);
  decode_op_fpr(s, id_dest, s->isa.instr.i.rd, false);
}


// --------- FSD/FSW ---------

static inline def_DHelper(F_S) {
  decode_op_r(s, id_src1, s->isa.instr.s.rs1, true);
  sword_t simm = (s->isa.instr.s.simm11_5 << 5) | s->isa.instr.s.imm4_0;
  decode_op_i(s, id_src2, simm, false);
  decode_op_fpr(s, id_dest, s->isa.instr.s.rs2, true);
}

// --------- fpr to gpr ---------

static inline def_DHelper(F_fpr_to_gpr){
  decode_op_fpr(s, id_src1, s->isa.instr.fp.rs1, true);
  decode_op_fpr(s, id_src2, s->isa.instr.fp.rs2, true);
  decode_op_r(s, id_dest, s->isa.instr.fp.rd, false);
  decode_fp_width(s);
}

// --------- gpr to fpr ---------

static inline def_DHelper(F_gpr_to_fpr){
  decode_op_r(s, id_src1, s->isa.instr.fp.rs1, true);
  decode_op_r(s, id_src2, s->isa.instr.fp.rs2, true);
  decode_op_fpr(s, id_dest, s->isa.instr.fp.rd, false);
  decode_fp_width(s);
}
#endif


// RVC

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
  id_src2->imm <<= 12;
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
#if 0
  if(is_fp)
    decode_op_fpr(s, id_dest, rd, false);
  else 
#endif
    decode_op_r(s, id_dest, rd, false);
}

static inline def_DHelper(C_LWSP) {
  decode_C_LxSP(s, 2, false);
}

static inline def_DHelper(C_LDSP) {
  decode_C_LxSP(s, 3, false);
}

#if 0
static inline def_DHelper(C_FLWSP) {
  decode_C_LxSP(s, 2, true);
}

static inline def_DHelper(C_FLDSP) {
  decode_C_LxSP(s, 3, true);
}
#endif

// ---------- CSS ---------- 

static inline void decode_C_SxSP(Decode *s, int rotate, bool is_fp) {
  uint32_t imm6 = BITS(s->isa.instr.val, 12, 7);
  decode_C_xxSP(s, imm6, rotate);
  uint32_t rs2 = BITS(s->isa.instr.val, 6, 2);
#if 0
  if(is_fp)
    decode_op_fpr(s, id_dest, rs2, true);
  else 
#endif
    decode_op_r(s, id_dest, rs2, true);
}

static inline def_DHelper(C_SWSP) {
  decode_C_SxSP(s, 2, false);
}

static inline def_DHelper(C_SDSP) {
  decode_C_SxSP(s, 3, false);
}

#if 0
static inline def_DHelper(C_FSWSP) {
  decode_C_SxSP(s, 2, true);
}

static inline def_DHelper(C_FSDSP) {
  decode_C_SxSP(s, 3, true);
}
#endif

// ---------- CIW ---------- 

static inline def_DHelper(C_ADDI4SPN) {
  decode_op_r(s, id_src1, 2, true);
  uint32_t instr = s->isa.instr.val;
  uint32_t imm9_6 = ror_imm(BITS(instr, 12, 7), 6, 4); // already at the right place
  uint32_t imm = imm9_6 | BITS(instr, 5, 5) << 3 | BITS(instr, 6, 6) << 2;
  Assert(imm != 0, "pc = " FMT_WORD, cpu.pc);
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
#if 0
  if(is_fp)
    decode_op_fpr(s, id_dest, creg2reg(BITS(instr, 4, 2)), is_store);
  else 
#endif
    decode_op_r(s, id_dest, creg2reg(BITS(instr, 4, 2)), is_store);
}

static inline def_DHelper(C_LW) {
  decode_C_ldst_common(s, 1, false, false);
}

static inline def_DHelper(C_LD) {
  decode_C_ldst_common(s, 2, false, false);
}

#if 0
static inline def_DHelper(C_FLD) {
  decode_C_ldst_common(s, 2, false, true);
}
#endif

// ---------- CS ---------- 

static inline def_DHelper(C_SW) {
  decode_C_ldst_common(s, 1, true, false);
}

static inline def_DHelper(C_SD) {
  decode_C_ldst_common(s, 2, true, false);
}

#if 0
static inline def_DHelper(C_FSD) {
  decode_C_ldst_common(s, 2, true, true);
}
#endif

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

static inline def_DHelper(C_J) {
  decode_CJ(s);
  decode_op_r(s, id_dest, 0, false);
}

static inline void decode_C_rs1_rs2_rd(Decode *s, bool is_rs1_zero, bool is_rs2_zero, bool is_rd_zero) {
  decode_op_r(s, id_src1, (is_rs1_zero ? 0 : BITS(s->isa.instr.val, 11, 7)), true);
  decode_op_r(s, id_src2, (is_rs2_zero ? 0 : BITS(s->isa.instr.val, 6, 2)), true);
  decode_op_r(s, id_dest, (is_rd_zero ? 0 : BITS(s->isa.instr.val, 11, 7)), false);
}

static inline def_DHelper(C_JR) {
  decode_op_r(s, id_src1, BITS(s->isa.instr.val, 11, 7), true);
  decode_op_i(s, id_src2, 0, true);
  decode_op_r(s, id_dest, 0, false);
}

static inline def_DHelper(C_MOV) {
  decode_C_rs1_rs2_rd(s, true, false, false);
}

static inline def_DHelper(C_ADD) {
  decode_C_rs1_rs2_rd(s, false, false, false);
}

static inline def_DHelper(C_JALR) {
  decode_op_r(s, id_src1, BITS(s->isa.instr.val, 11, 7), true);
  decode_op_i(s, id_src2, 0, true);
  decode_op_r(s, id_dest, 1, false);
}

def_THelper(load) {
  print_Dop(id_src1->str, OP_STR_SIZE, "%ld(%s)", id_src2->imm, reg_name(s->isa.instr.i.rs1, 4));
  switch (s->isa.instr.i.funct3) {
    TAB(0, lb)  TAB(1, lh)  TAB(2, lw)  TAB(3, ld)
    TAB(4, lbu) TAB(5, lhu) TAB(6, lwu)
  }
  return EXEC_ID_inv;
}

def_THelper(store) {
  print_Dop(id_src1->str, OP_STR_SIZE, "%ld(%s)", id_src2->imm, reg_name(s->isa.instr.i.rs1, 4));
  switch (s->isa.instr.i.funct3) {
    TAB(0, sb)  TAB(1, sh)  TAB(2, sw)  TAB(3, sd)
  }
  return EXEC_ID_inv;
}

def_THelper(op_imm) {
  if ((s->isa.instr.r.funct7 & ~0x1) == 32) {
    switch (s->isa.instr.r.funct3) { TAB(5, srai) }
  }
  switch (s->isa.instr.i.funct3) {
    TAB(0, addi)  TAB(1, slli)  TAB(2, slti) TAB(3, sltui)
    TAB(4, xori)  TAB(5, srli)  TAB(6, ori)  TAB(7, andi)
  }
  return EXEC_ID_inv;
};

def_THelper(op_imm32) {
  if (s->isa.instr.r.funct7 == 32) {
    switch (s->isa.instr.r.funct3) { TAB(5, sraiw) }
  }
  switch (s->isa.instr.i.funct3) {
    TAB(0, addiw) TAB(1, slliw) TAB(5, srliw)
  }
  return EXEC_ID_inv;
}

def_THelper(op) {
  if (s->isa.instr.r.funct7 == 32) {
    switch (s->isa.instr.r.funct3) {
      TAB(0, sub) TAB(5, sra)
    }
    return EXEC_ID_inv;
  }
#define pair(x, y) (((x) << 3) | (y))
  int index = pair(s->isa.instr.r.funct7 & 1, s->isa.instr.r.funct3);
  switch (index) {
    TAB(pair(0, 0), add)  TAB(pair(0, 1), sll)  TAB(pair(0, 2), slt)  TAB(pair(0, 3), sltu)
    TAB(pair(0, 4), xor)  TAB(pair(0, 5), srl)  TAB(pair(0, 6), or)   TAB(pair(0, 7), and)
    TAB(pair(1, 0), mul)  TAB(pair(1, 1), mulh) TAB(pair(1, 2),mulhsu)TAB(pair(1, 3), mulhu)
    TAB(pair(1, 4), div)  TAB(pair(1, 5), divu) TAB(pair(1, 6), rem)  TAB(pair(1, 7), remu)
  }
  return EXEC_ID_inv;
}

def_THelper(op32) {
  if (s->isa.instr.r.funct7 == 32) {
    switch (s->isa.instr.r.funct3) {
      TAB(0, subw) TAB(5, sraw)
    }
    return EXEC_ID_inv;
  }
  int index = pair(s->isa.instr.r.funct7 & 1, s->isa.instr.r.funct3);
  switch (index) {
    TAB(pair(0, 0), addw) TAB(pair(0, 1), sllw)
                          TAB(pair(0, 5), srlw)
    TAB(pair(1, 0), mulw)
    TAB(pair(1, 4), divw) TAB(pair(1, 5), divuw) TAB(pair(1, 6), remw)  TAB(pair(1, 7), remuw)
  }
  return EXEC_ID_inv;
#undef pair
}

def_THelper(branch) {
  switch (s->isa.instr.i.funct3) {
    TAB(0, beq)   TAB(1, bne)
    TAB(4, blt)   TAB(5, bge)   TAB(6, bltu)   TAB(7, bgeu)
  }
  return EXEC_ID_inv;
};

def_THelper(priv) {
  switch (s->isa.instr.csr.csr) {
    TAB(0, ecall)  TAB (0x102, sret)  TAB (0x105, wfi) TAB (0x120, sfence_vma)
    TAB(0x302, mret)
  }
  return EXEC_ID_inv;
};

def_THelper(system) {
  switch (s->isa.instr.i.funct3) {
    IDTAB(0, csr, priv)  IDTAB(1, csr, csrrw)  IDTAB(2, csr, csrrs)  IDTAB(3, csr, csrrc)
                         IDTAB(5, csri, csrrwi)IDTAB(6, csri, csrrsi)IDTAB(7, csri, csrrci)
  }
  return EXEC_ID_inv;
};

def_THelper(jal_dispatch) {
  if (s->isa.instr.j.rd != 0) return table_jal(s);
  return table_j(s);
}

def_THelper(jalr_dispatch) {
  if (s->isa.instr.i.rd != 0) return table_jalr(s);
  return table_jr(s);
}

#if 0
static inline def_EHelper(fp_load) {
  switch (s->isa.instr.i.funct3) {
    EXW(2, fp_ld, 4) EXW(3, fp_ld, 8)
    default: exec_inv(s);
  }
}

static inline def_EHelper(fp_store) {
  switch (s->isa.instr.s.funct3) {
    EXW(2, fp_st, 4) EXW(3, fp_st, 8)
    default: exec_inv(s);
  }
}

static inline def_EHelper(op_fp){
  switch (s->isa.instr.fp.funct5) {
    EX(0, fadd) EX(1, fsub) EX(2, fmul) EX(3, fdiv)
    EX(4, fsgnj) EX(5, fmin_fmax)
    EX(8, fcvt_F_to_F) EX(11, fsqrt) 
    IDEX(20, F_fpr_to_gpr, fcmp) 
    IDEX(24, F_fpr_to_gpr, fcvt_F_to_G) IDEX(26, F_gpr_to_fpr, fcvt_G_to_F)
    IDEX(28, F_fpr_to_gpr, fmv_F_to_G) IDEX(30, F_gpr_to_fpr, fmv_G_to_F)
    default: exec_inv(s);
  }
}

static inline def_EHelper(atomic) {
  cpu.amo = true;
  uint32_t funct5 = s->isa.instr.r.funct7 >> 2;
  if (funct5 == 2) cpu.amo = false; // lr is not a store
  set_width(s, 1 << s->isa.instr.r.funct3);
  switch (funct5) {
    EX(0x00, amoadd) EX(0x01, amoswap) EX(0x02, lr) EX(0x03, sc)
    EX(0x04, amoxor)
    EX(0x0c, amoand)
    EX(0x08, amoor)
    EX(0x10, amomin)
    EX(0x14, amomax)
    EX(0x18, amominu)
    EX(0x1c, amomaxu)
  }
  cpu.amo = false;
}

static inline def_EHelper(fp) {
  rtl_trap(s, cpu.pc, EX_II);
}
#endif

def_THelper(main) {
  switch (s->isa.instr.i.opcode6_2) {
    IDTAB(000, I, load)   /*IDTAB(001, F_I, fp_load)*/                         TAB  (003, fence)
    IDTAB(004, I, op_imm) IDTAB(005, auipc, auipc)      IDTAB(006, I, op_imm32)
    IDTAB(010, S, store)  /*IDTAB(011, F_S, fp_store)*/                        /*IDTAB(013, R, atomic)*/
    IDTAB(014, R, op)     IDTAB(015, U, lui)            IDTAB(016, R, op32)
    /*IDTAB(020, F_R, fmadd)IDTAB(021, F_R, fmsub)        IDTAB(022, F_R, fnmsub)IDTAB(023, F_R, fnmadd)
    IDTAB(024, F_R, op_fp)*/
    IDTAB(030, B, branch) IDTAB(031, I, jalr_dispatch)  TAB  (032, nemu_trap)  IDTAB(033, J, jal_dispatch)
    TAB  (034, system)                                  /*IDTAB(036, R, rocc3)*/
  }
  return table_inv(s);
};

// RVC

def_THelper(misc) {
  uint32_t instr = s->isa.instr.val;
  uint32_t bits12not0 = (BITS(instr, 12, 12) != 0);
  uint32_t bits11_7not0 = (BITS(instr, 11, 7) != 0);
  uint32_t bits6_2not0 = (BITS(instr, 6, 2) != 0);
  uint32_t op = (bits12not0 << 2) | (bits11_7not0 << 1) | bits6_2not0;
  switch (op) {
    IDTAB(0b010, C_JR, jalr)
    IDTAB(0b011, C_MOV, add)
    IDTAB(0b110, C_JALR, jalr)
    IDTAB(0b111, C_ADD, add)
  }
  return EXEC_ID_inv;
}

def_THelper(lui_addi16sp) {
  uint32_t rd = BITS(s->isa.instr.val, 11, 7);
  assert(rd != 0);
  switch (rd) {
    IDTAB(2, C_ADDI16SP, addi)
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
      IDTAB(0, CS, sub) IDTAB(1, CS, xor) IDTAB(2, CS, or)  IDTAB(3, CS, and)
      IDTAB(4, CS, subw)IDTAB(5, CS, addw)
    }
  } else {
    switch (op) {
      IDTAB(0, CB_shift, srli)
      IDTAB(1, CB_shift, srai)
      IDTAB(2, CB_andi, andi)
    }
  }
  return EXEC_ID_inv;
}

def_THelper(rvc) {
  uint32_t rvc_opcode = (s->isa.instr.r.opcode1_0 << 3) | BITS(s->isa.instr.val, 15, 13);
  switch (rvc_opcode) {
    IDTAB(000, C_ADDI4SPN, addi) /*IDTABW(001, C_FLD, fp_ld, 8)*/ IDTAB(002, C_LW, ld)    IDTAB(003, C_LD, ld)
                                 /*IDTABW(005, C_FSD, fp_st, 8)*/ IDTAB(006, C_SW, sw)    IDTAB(007, C_SD, sd)
    IDTAB(010, CI_simm, addi)    IDTAB(011, CI_simm, addiw)    IDTAB(012, C_LI, addi)     TAB  (013, lui_addi16sp)
    TAB  (014, misc_alu)         IDTAB(015, C_J, jal)          IDTAB(016, CB, beq)        IDTAB(017, CB, bne)
    IDTAB(020, CI_uimm, slli)    /*IDTABW(021, C_FLDSP, fp_ld, 8)*/ IDTAB(022, C_LWSP, lw)IDTAB(023, C_LDSP, ld)
    TAB  (024, misc)             /*IDTABW(025, C_FSDSP, fp_st, 8)*/ IDTAB(026, C_SWSP, sw)IDTAB(027, C_SDSP, sd)
  }
  return table_inv(s);
};

int isa_fetch_decode(Decode *s) {
  int idx = EXEC_ID_inv;

  if ((s->pc & 0xfff) == 0xffe) {
    // instruction may accross page boundary
    uint32_t lo = instr_fetch(&s->snpc, 2);
    //return_on_mem_ex();
    s->isa.instr.val = lo & 0xffff;
    if (s->isa.instr.r.opcode1_0 != 0x3) {
      // this is an RVC instruction
      goto rvc;
    }
    // this is a 4-byte instruction, should fetch the MSB part
    // NOTE: The fetch here may cause IPF.
    // If it is the case, we should have mepc = xxxffe and mtval = yyy000.
    // Refer to `mtval` in the privileged manual for more details.
    uint32_t hi = instr_fetch(&s->snpc, 2);
    s->isa.instr.val |= ((hi & 0xffff) << 16);
  } else {
    // in-page instructions, fetch 4 byte and
    // see whether it is an RVC instruction later
    s->isa.instr.val = instr_fetch(&s->snpc, 4);
  }

  //return_on_mem_ex();

  if (s->isa.instr.r.opcode1_0 == 0x3) {
    idx = table_main(s);
  } else {
    // RVC instructions are only 2-byte
    s->snpc -= 2;
rvc: idx = table_rvc(s);
  }

  s->type = INSTR_TYPE_N;
  switch (idx) {
    case EXEC_ID_j:
    case EXEC_ID_jal: s->jnpc = id_src1->imm; s->type = INSTR_TYPE_J; break;
    case EXEC_ID_beq:
    case EXEC_ID_bne:
    case EXEC_ID_blt:
    case EXEC_ID_bge:
    case EXEC_ID_bltu:
    case EXEC_ID_bgeu: s->jnpc = id_dest->imm; s->type = INSTR_TYPE_B; break;
    case EXEC_ID_jr:
    case EXEC_ID_jalr: s->type = INSTR_TYPE_I;
  }

  return idx;
}
