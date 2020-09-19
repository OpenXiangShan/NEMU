#include <cpu/exec.h>
#include "rtl.h"

// decode operand helper
#define make_DopHelper(name) \
  void concat(decode_op_, name) (DecodeExecState *s, Operand *op, uint64_t val, bool load_val)

static inline make_DopHelper(i) {
  op->type = OP_TYPE_IMM;
  op->imm = val;

  print_Dop(op->str, OP_STR_SIZE, "%ld", op->imm);
}

static inline make_DopHelper(r) {
  op->type = OP_TYPE_REG;
  op->reg = val;
  op->preg = &reg_l(val);

  print_Dop(op->str, OP_STR_SIZE, "%s", reg_name(op->reg, 4));
}

static inline make_DopHelper(fpr){
  op->type = OP_TYPE_REG;
  op->reg = val;
  op->preg = &fpreg_l(val);

  print_Dop(op->str, OP_STR_SIZE, "%s", fpreg_name(op->reg, 4));
}

static inline make_DHelper(I) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, (sword_t)s->isa.instr.i.simm11_0, true);
  decode_op_r(s, id_dest, s->isa.instr.i.rd, false);
}

static inline make_DHelper(R) {
  decode_op_r(s, id_src1, s->isa.instr.r.rs1, true);
  decode_op_r(s, id_src2, s->isa.instr.r.rs2, true);
  decode_op_r(s, id_dest, s->isa.instr.r.rd, false);
}

static inline make_DHelper(U) {
  decode_op_i(s, id_src2, (sword_t)s->isa.instr.u.simm31_12 << 12, true);
  decode_op_r(s, id_dest, s->isa.instr.u.rd, false);

  print_Dop(id_src2->str, OP_STR_SIZE, "0x%x", s->isa.instr.u.simm31_12);
}

static inline make_DHelper(J) {
  sword_t offset = (s->isa.instr.j.simm20 << 20) | (s->isa.instr.j.imm19_12 << 12) |
    (s->isa.instr.j.imm11 << 11) | (s->isa.instr.j.imm10_1 << 1);
  s->jmp_pc = cpu.pc + offset;
  print_Dop(id_src1->str, OP_STR_SIZE, "0x%lx", s->jmp_pc);

  decode_op_r(s, id_dest, s->isa.instr.j.rd, false);
}

static inline make_DHelper(B) {
  sword_t offset = (s->isa.instr.b.simm12 << 12) | (s->isa.instr.b.imm11 << 11) |
    (s->isa.instr.b.imm10_5 << 5) | (s->isa.instr.b.imm4_1 << 1);
  s->jmp_pc = cpu.pc + offset;
  print_Dop(id_dest->str, OP_STR_SIZE, "0x%lx", s->jmp_pc);

  decode_op_r(s, id_src1, s->isa.instr.b.rs1, true);
  decode_op_r(s, id_src2, s->isa.instr.b.rs2, true);
}

static inline make_DHelper(S) {
  decode_op_r(s, id_src1, s->isa.instr.s.rs1, true);
  sword_t simm = (s->isa.instr.s.simm11_5 << 5) | s->isa.instr.s.imm4_0;
  decode_op_i(s, id_src2, simm, true);
  decode_op_r(s, id_dest, s->isa.instr.s.rs2, true);
}

static inline make_DHelper(csr) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.csr.csr, true);
  decode_op_r(s, id_dest, s->isa.instr.i.rd, false);
}

static inline make_DHelper(csri) {
  decode_op_i(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.csr.csr, true);
  decode_op_r(s, id_dest, s->isa.instr.i.rd, false);
}


// RVF RVD

static inline void decode_fp_width(DecodeExecState *s) {
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

static inline make_DHelper(F_R) {
  decode_op_fpr(s, id_src1, s->isa.instr.fp.rs1, true);
  decode_op_fpr(s, id_src2, s->isa.instr.fp.rs2, true);
  decode_op_fpr(s, id_dest, s->isa.instr.fp.rd, false);
  decode_fp_width(s);
}

// --------- FLD/FLW --------- 

static inline make_DHelper(F_I) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, (sword_t)s->isa.instr.i.simm11_0, true);
  decode_op_fpr(s, id_dest, s->isa.instr.i.rd, false);
}


// --------- FSD/FSW ---------

static inline make_DHelper(F_S) {
  decode_op_r(s, id_src1, s->isa.instr.s.rs1, true);
  sword_t simm = (s->isa.instr.s.simm11_5 << 5) | s->isa.instr.s.imm4_0;
  decode_op_i(s, id_src2, simm, true);
  decode_op_fpr(s, id_dest, s->isa.instr.s.rs2, true);
}

// --------- fpr to gpr ---------

static inline make_DHelper(F_fpr_to_gpr){
  decode_op_fpr(s, id_src1, s->isa.instr.fp.rs1, true);
  decode_op_fpr(s, id_src2, s->isa.instr.fp.rs2, true);
  decode_op_r(s, id_dest, s->isa.instr.fp.rd, false);
  decode_fp_width(s);
}

// --------- gpr to fpr ---------

static inline make_DHelper(F_gpr_to_fpr){
  decode_op_r(s, id_src1, s->isa.instr.fp.rs1, true);
  decode_op_r(s, id_src2, s->isa.instr.fp.rs2, true);
  decode_op_fpr(s, id_dest, s->isa.instr.fp.rd, false);
  decode_fp_width(s);
}


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

static inline void decode_op_C_imm6(DecodeExecState *s, uint32_t imm6, bool sign, int shift, int rotate) {
  uint64_t imm = (sign ? SEXT(imm6, 6) << shift : ror_imm(imm6, 6, rotate));
  decode_op_i(s, id_src2, imm, true);
}

static inline void decode_op_C_rd_rs1(DecodeExecState *s, bool creg) {
  uint32_t instr = s->isa.instr.val;
  uint32_t rd_rs1 = (creg ? creg2reg(BITS(instr, 9, 7)) : BITS(instr, 11, 7));
  decode_op_r(s, id_src1, rd_rs1, true);
  decode_op_r(s, id_dest, rd_rs1, false);
}

// creg = false for CI, true for CB_shift and CB_andi
static inline void decode_op_rd_rs1_imm6(DecodeExecState *s, bool sign, int shift, int rotate, bool creg) {
  uint32_t instr = s->isa.instr.val;
  uint32_t imm6 = (BITS(instr, 12, 12) << 5) | BITS(instr, 6, 2);
  decode_op_C_imm6(s, imm6, sign, shift, rotate);
  decode_op_C_rd_rs1(s, creg);
}

// ---------- CR ---------- 

static inline make_DHelper(CR) {
  decode_op_C_rd_rs1(s, false);
  uint32_t rs2 = BITS(s->isa.instr.val, 6, 2);
  decode_op_r(s, id_src2, rs2, true);
}

// ---------- CI ---------- 

static inline make_DHelper(CI_simm) {
  decode_op_rd_rs1_imm6(s, true, 0, 0, false);
}

static inline make_DHelper(CI_simm_lui) {
  decode_CI_simm(s);
  id_src2->imm <<= 12;
}

// for shift
static inline make_DHelper(CI_uimm) {
  decode_op_rd_rs1_imm6(s, false, 0, 0, false);
}

static inline make_DHelper(C_LI) {
  decode_CI_simm(s);
  decode_op_r(s, id_src1, 0, true);
}

static inline make_DHelper(C_ADDI16SP) {
  decode_op_r(s, id_src1, 2, true);
  uint32_t instr = s->isa.instr.val;
  sword_t simm = (SEXT(BITS(instr, 12, 12), 1) << 9) | (BITS(instr, 4, 3) << 7) |
    (BITS(instr, 5, 5) << 6) | (BITS(instr, 2, 2) << 5) | (BITS(instr, 6, 6) << 4);
  assert(simm != 0);
  decode_op_i(s, id_src2, simm, true);
  decode_op_r(s, id_dest, 2, false);
}

// SP-relative load/store
static inline void decode_C_xxSP(DecodeExecState *s, uint32_t imm6, int rotate) {
  decode_op_r(s, id_src1, 2, true);
  decode_op_C_imm6(s, imm6, false, 0, rotate);
}

static inline void decode_C_LxSP(DecodeExecState *s, int rotate, bool is_fp) {
  uint32_t imm6 = (BITS(s->isa.instr.val, 12, 12) << 5) | BITS(s->isa.instr.val, 6, 2);
  decode_C_xxSP(s, imm6, rotate);
  uint32_t rd = BITS(s->isa.instr.val, 11, 7);
  if(is_fp)
    decode_op_fpr(s, id_dest, rd, false);
  else 
    decode_op_r(s, id_dest, rd, false);
}

static inline make_DHelper(C_LWSP) {
  decode_C_LxSP(s, 2, false);
}

static inline make_DHelper(C_LDSP) {
  decode_C_LxSP(s, 3, false);
}

static inline make_DHelper(C_FLWSP) {
  decode_C_LxSP(s, 2, true);
}

static inline make_DHelper(C_FLDSP) {
  decode_C_LxSP(s, 3, true);
}

// ---------- CSS ---------- 

static inline void decode_C_SxSP(DecodeExecState *s, int rotate, bool is_fp) {
  uint32_t imm6 = BITS(s->isa.instr.val, 12, 7);
  decode_C_xxSP(s, imm6, rotate);
  uint32_t rs2 = BITS(s->isa.instr.val, 6, 2);
  if(is_fp)
    decode_op_fpr(s, id_dest, rs2, true);
  else 
    decode_op_r(s, id_dest, rs2, true);
}

static inline make_DHelper(C_SWSP) {
  decode_C_SxSP(s, 2, false);
}

static inline make_DHelper(C_SDSP) {
  decode_C_SxSP(s, 3, false);
}

static inline make_DHelper(C_FSWSP) {
  decode_C_SxSP(s, 2, true);
}

static inline make_DHelper(C_FSDSP) {
  decode_C_SxSP(s, 3, true);
}

// ---------- CIW ---------- 

static inline make_DHelper(C_ADDI4SPN) {
  decode_op_r(s, id_src1, 2, true);
  uint32_t instr = s->isa.instr.val;
  uint32_t imm9_6 = ror_imm(BITS(instr, 12, 7), 6, 4); // already at the right place
  uint32_t imm = imm9_6 | BITS(instr, 5, 5) << 3 | BITS(instr, 6, 6) << 2;
  Assert(imm != 0, "pc = " FMT_WORD, cpu.pc);
  decode_op_i(s, id_src2, imm, true);
  decode_op_r(s, id_dest, creg2reg(BITS(instr, 4, 2)), false);
}

// ---------- CL ---------- 

// load/store
static inline void decode_C_ldst_common(DecodeExecState *s, int rotate, bool is_store, bool is_fp) {
  uint32_t instr = s->isa.instr.val;
  decode_op_r(s, id_src1, creg2reg(BITS(instr, 9, 7)), true);
  uint32_t imm5 = (BITS(instr, 12, 10) << 2) | BITS(instr, 6, 5);
  uint32_t imm = ror_imm(imm5, 5, rotate) << 1;
  decode_op_i(s, id_src2, imm, true);
  if(is_fp)
    decode_op_fpr(s, id_dest, creg2reg(BITS(instr, 4, 2)), is_store);
  else 
    decode_op_r(s, id_dest, creg2reg(BITS(instr, 4, 2)), is_store);
}

static inline make_DHelper(C_LW) {
  decode_C_ldst_common(s, 1, false, false);
}

static inline make_DHelper(C_LD) {
  decode_C_ldst_common(s, 2, false, false);
}

static inline make_DHelper(C_FLD) {
  decode_C_ldst_common(s, 2, false, true);
}

// ---------- CS ---------- 

static inline make_DHelper(C_SW) {
  decode_C_ldst_common(s, 1, true, false);
}

static inline make_DHelper(C_SD) {
  decode_C_ldst_common(s, 2, true, false);
}

static inline make_DHelper(C_FSD) {
  decode_C_ldst_common(s, 2, true, true);
}

static inline make_DHelper(CS) {
  decode_op_C_rd_rs1(s, true);
  uint32_t rs2 = creg2reg(BITS(s->isa.instr.val, 4, 2));
  decode_op_r(s, id_src2, rs2, true);
}

// ---------- CB ---------- 

static inline make_DHelper(CB) {
  uint32_t instr = s->isa.instr.val;
  sword_t simm8 = SEXT(BITS(instr, 12, 12), 1);
  uint32_t imm7_6 = BITS(instr, 6, 5);
  uint32_t imm5 = BITS(instr, 2, 2);
  uint32_t imm4_3 = BITS(instr, 11, 10);
  uint32_t imm2_1 = BITS(instr, 4, 3);
  sword_t offset = (simm8 << 8) | (imm7_6 << 6) | (imm5 << 5) | (imm4_3 << 3) | (imm2_1 << 1);

  s->jmp_pc = cpu.pc + offset;
  decode_op_i(s, id_dest, s->jmp_pc, true);

  decode_op_r(s, id_src1, creg2reg(BITS(instr, 9, 7)), true);
  decode_op_r(s, id_src2, 0, true);
}

static inline make_DHelper(CB_shift) {
  decode_op_rd_rs1_imm6(s, false, 0, 0, true);
}

static inline make_DHelper(CB_andi) {
  decode_op_rd_rs1_imm6(s, true, 0, 0, true);
}

// ---------- CJ ---------- 

static inline make_DHelper(CJ) {
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
  s->jmp_pc = cpu.pc + offset;
  decode_op_i(s, id_src1, s->jmp_pc, true);
}

static inline make_DHelper(C_J) {
  decode_CJ(s);
  decode_op_r(s, id_dest, 0, false);
}

static inline void decode_C_rs1_rs2_rd(DecodeExecState *s, bool is_rs1_zero, bool is_rs2_zero, bool is_rd_zero) {
  decode_op_r(s, id_src1, (is_rs1_zero ? 0 : BITS(s->isa.instr.val, 11, 7)), true);
  decode_op_r(s, id_src2, (is_rs2_zero ? 0 : BITS(s->isa.instr.val, 6, 2)), true);
  decode_op_r(s, id_dest, (is_rd_zero ? 0 : BITS(s->isa.instr.val, 11, 7)), false);
}

static inline make_DHelper(C_JR) {
  decode_op_r(s, id_src1, BITS(s->isa.instr.val, 11, 7), true);
  decode_op_i(s, id_src2, 0, true);
  decode_op_r(s, id_dest, 0, false);
}

static inline make_DHelper(C_MOV) {
  decode_C_rs1_rs2_rd(s, true, false, false);
}

static inline make_DHelper(C_ADD) {
  decode_C_rs1_rs2_rd(s, false, false, false);
}

static inline make_DHelper(C_JALR) {
  decode_op_r(s, id_src1, BITS(s->isa.instr.val, 11, 7), true);
  decode_op_i(s, id_src2, 0, true);
  decode_op_r(s, id_dest, 1, false);
}
