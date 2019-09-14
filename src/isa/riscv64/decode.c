#include "cpu/decode.h"
#include "rtl/rtl.h"

// decode operand helper
#define make_DopHelper(name) void concat(decode_op_, name) (Operand *op, uint64_t val, bool load_val)

static inline make_DopHelper(i) {
  op->type = OP_TYPE_IMM;
  op->imm = val;
  rtl_li(&op->val, op->imm);

  print_Dop(op->str, OP_STR_SIZE, "%ld", op->imm);
}

static inline make_DopHelper(r) {
  op->type = OP_TYPE_REG;
  op->reg = val;
  if (load_val) {
    rtl_lr(&op->val, op->reg, 4);
  }

  print_Dop(op->str, OP_STR_SIZE, "%s", reg_name(op->reg, 4));
}

make_DHelper(I) {
  decode_op_r(id_src, decinfo.isa.instr.rs1, true);
  decode_op_i(id_src2, (sword_t)decinfo.isa.instr.simm11_0, true);
  decode_op_r(id_dest, decinfo.isa.instr.rd, false);
}

make_DHelper(R) {
  decode_op_r(id_src, decinfo.isa.instr.rs1, true);
  decode_op_r(id_src2, decinfo.isa.instr.rs2, true);
  decode_op_r(id_dest, decinfo.isa.instr.rd, false);
}

make_DHelper(U) {
  // shift at execute stage
  decode_op_i(id_src2, (sword_t)decinfo.isa.instr.simm31_12, true);
  decode_op_r(id_dest, decinfo.isa.instr.rd, false);

  print_Dop(id_src2->str, OP_STR_SIZE, "0x%x", decinfo.isa.instr.simm31_12);
}

make_DHelper(J) {
  sword_t offset = (decinfo.isa.instr.simm20 << 20) | (decinfo.isa.instr.imm19_12 << 12) |
    (decinfo.isa.instr.imm11_ << 11) | (decinfo.isa.instr.imm10_1 << 1);
  decinfo.jmp_pc = cpu.pc + offset;
  decode_op_i(id_src, decinfo.jmp_pc, true);
  print_Dop(id_src->str, OP_STR_SIZE, "0x%lx", decinfo.jmp_pc);

  decode_op_r(id_dest, decinfo.isa.instr.rd, false);
}

make_DHelper(B) {
  sword_t offset = (decinfo.isa.instr.simm12 << 12) | (decinfo.isa.instr.imm11 << 11) |
    (decinfo.isa.instr.imm10_5 << 5) | (decinfo.isa.instr.imm4_1 << 1);
  decinfo.jmp_pc = cpu.pc + offset;
  decode_op_i(id_dest, decinfo.jmp_pc, true);
  print_Dop(id_dest->str, OP_STR_SIZE, "0x%lx", decinfo.jmp_pc);

  decode_op_r(id_src, decinfo.isa.instr.rs1, true);
  decode_op_r(id_src2, decinfo.isa.instr.rs2, true);
}

make_DHelper(ld) {
  decode_op_r(id_src, decinfo.isa.instr.rs1, true);
  decode_op_i(id_src2, decinfo.isa.instr.simm11_0, true);

  print_Dop(id_src->str, OP_STR_SIZE, "%ld(%s)", id_src2->val, reg_name(id_src->reg, 4));

  rtl_add(&id_src->addr, &id_src->val, &id_src2->val);

  decode_op_r(id_dest, decinfo.isa.instr.rd, false);
}

make_DHelper(st) {
  decode_op_r(id_src, decinfo.isa.instr.rs1, true);
  sword_t simm = (decinfo.isa.instr.simm11_5 << 5) | decinfo.isa.instr.imm4_0;
  decode_op_i(id_src2, simm, true);

  print_Dop(id_src->str, OP_STR_SIZE, "%ld(%s)", id_src2->val, reg_name(id_src->reg, 4));

  rtl_add(&id_src->addr, &id_src->val, &id_src2->val);

  decode_op_r(id_dest, decinfo.isa.instr.rs2, true);
}

make_DHelper(csr) {
  decode_op_r(id_src, decinfo.isa.instr.rs1, true);
  decode_op_i(id_src2, decinfo.isa.instr.csr, true);
  decode_op_r(id_dest, decinfo.isa.instr.rd, false);
}

// RVC

#define creg2reg(creg) (creg + 8)

make_DHelper(CR) {
  decode_op_r(id_src, creg2reg(decinfo.isa.instr.c_rd_rs1_), true);
  decode_op_r(id_src2, creg2reg(decinfo.isa.instr.c_rs2_), true);
  decode_op_r(id_dest, creg2reg(decinfo.isa.instr.c_rd_rs1_), false);
}

make_DHelper(CB) {
  sword_t simm8 = decinfo.isa.instr.c_simm12;
  uint32_t imm7_6 = decinfo.isa.instr.c_imm6_2 >> 3;
  uint32_t imm5 = decinfo.isa.instr.c_imm6_2 & 0x1;
  uint32_t imm4_3 = decinfo.isa.instr.c_imm12_10 & 0x3;
  uint32_t imm2_1 = (decinfo.isa.instr.c_imm6_2 >> 1) & 0x3;
  sword_t offset = (simm8 << 8) | (imm7_6 << 6) | (imm5 << 5) | (imm4_3 << 3) | (imm2_1 << 1);

  decinfo.jmp_pc = cpu.pc + offset;
  decode_op_i(id_dest, decinfo.jmp_pc, true);

  decode_op_r(id_src, creg2reg(decinfo.isa.instr.c_rd_rs1_), true);
  decode_op_r(id_src2, 0, true);
}

static make_DHelper(CJ) {
  sword_t simm11 = decinfo.isa.instr.c_simm12;
  uint32_t imm10  = (decinfo.isa.instr.c_target >> 6) & 0x1;
  uint32_t imm9_8 = (decinfo.isa.instr.c_target >> 7) & 0x3;
  uint32_t imm7   = (decinfo.isa.instr.c_target >> 4) & 0x1;
  uint32_t imm6   = (decinfo.isa.instr.c_target >> 5) & 0x1;
  uint32_t imm5   = (decinfo.isa.instr.c_target >> 0) & 0x1;
  uint32_t imm4   = (decinfo.isa.instr.c_target >> 9) & 0x1;
  uint32_t imm3_1 = (decinfo.isa.instr.c_target >> 1) & 0x7;

  sword_t offset = (simm11 << 11) | (imm10 << 10) | (imm9_8 << 8) |
    (imm7 << 7) | (imm6 << 6) | (imm5 << 5) | (imm4 << 4) | (imm3_1 << 1);
  decinfo.jmp_pc = cpu.pc + offset;
  decode_op_i(id_src, decinfo.jmp_pc, true);
}

make_DHelper(C_SDSP) {
  decode_op_r(id_src, 2, true);
  uint32_t imm8_6 = (decinfo.isa.instr.c_imm12_7 & 0x7);
  uint32_t imm5_3 = (decinfo.isa.instr.c_imm12_7 >> 3);
  word_t imm = (imm8_6 << 6) | (imm5_3 << 3);
  decode_op_i(id_src2, imm, true);

  rtl_add(&id_src->addr, &id_src->val, &id_src2->val);

  decode_op_r(id_dest, decinfo.isa.instr.c_rs2, true);

  decinfo.width = 8;
}

make_DHelper(C_SWSP) {
  decode_op_r(id_src, 2, true);
  uint32_t imm7_6 = (decinfo.isa.instr.c_imm12_7 & 0x3);
  uint32_t imm5_2 = (decinfo.isa.instr.c_imm12_7 >> 2);
  word_t imm = (imm7_6 << 6) | (imm5_2 << 2);
  decode_op_i(id_src2, imm, true);

  rtl_add(&id_src->addr, &id_src->val, &id_src2->val);

  decode_op_r(id_dest, decinfo.isa.instr.c_rs2, true);

  decinfo.width = 4;
}

make_DHelper(C_LDSP) {
  decode_op_r(id_src, 2, true);
  uint32_t imm8_6 = (decinfo.isa.instr.c_imm6_2 & 0x7);
  uint32_t imm5 = (decinfo.isa.instr.c_simm12 & 0x1);
  uint32_t imm4_3 = (decinfo.isa.instr.c_imm6_2 >> 3);
  word_t imm = (imm8_6 << 6) | (imm5 << 5) | (imm4_3 << 3);
  decode_op_i(id_src2, imm, true);

  rtl_add(&id_src->addr, &id_src->val, &id_src2->val);

  decode_op_r(id_dest, decinfo.isa.instr.c_rd_rs1, true);
  assert(decinfo.isa.instr.c_rd_rs1 != 0);

  decinfo.width = 8;
}

make_DHelper(C_LWSP) {
  decode_op_r(id_src, 2, true);
  uint32_t imm7_6 = (decinfo.isa.instr.c_imm6_2 & 0x3);
  uint32_t imm5 = (decinfo.isa.instr.c_simm12 & 0x1);
  uint32_t imm4_2 = (decinfo.isa.instr.c_imm6_2 >> 2);
  word_t imm = (imm7_6 << 6) | (imm5 << 5) | (imm4_2 << 2);
  decode_op_i(id_src2, imm, true);

  rtl_add(&id_src->addr, &id_src->val, &id_src2->val);

  decode_op_r(id_dest, decinfo.isa.instr.c_rd_rs1, true);
  assert(decinfo.isa.instr.c_rd_rs1 != 0);

  decinfo.width = 4;
}

static void decode_C_xxx_imm_rd(bool is_rs1_zero, bool is_reg_compress) {
  int reg = (is_reg_compress ? creg2reg(decinfo.isa.instr.c_rd_rs1_) : decinfo.isa.instr.c_rd_rs1);
  decode_op_r(id_src, (is_rs1_zero ? 0 : reg), true);
  sword_t simm = (decinfo.isa.instr.c_simm12 << 5) | decinfo.isa.instr.c_imm6_2;
  decode_op_i(id_src2, simm, true);
  decode_op_r(id_dest, reg, false);
}

make_DHelper(C_0_imm_rd) {
  decode_C_xxx_imm_rd(true, false);
}

make_DHelper(C_rs1_imm_rd) {
  decode_C_xxx_imm_rd(false, false);
}

make_DHelper(C_rs1__imm_rd_) {
  decode_C_xxx_imm_rd(false, true);
}

static void decode_C_xxx_xxx_xxx(bool is_rs1_zero, bool is_rs2_zero, bool is_rd_zero) {
  decode_op_r(id_src, (is_rs1_zero ? 0 : decinfo.isa.instr.c_rd_rs1), true);
  decode_op_r(id_src2, (is_rs2_zero ? 0 : decinfo.isa.instr.c_rs2), true);
  decode_op_r(id_dest, (is_rd_zero ? 0 : decinfo.isa.instr.c_rd_rs1), false);
}

make_DHelper(C_0_rs2_rd) {
  decode_C_xxx_xxx_xxx(true, false, false);
}

make_DHelper(C_rs1_rs2_0) {
  decode_C_xxx_xxx_xxx(false, false, true);
}

make_DHelper(C_rs1_rs2_rd) {
  decode_C_xxx_xxx_xxx(false, false, false);
}

make_DHelper(C_ADDI16SP) {
  decode_op_r(id_src, 2, true);
  sword_t simm9 = decinfo.isa.instr.c_simm12;
  uint32_t imm8_7 = ((decinfo.isa.instr.c_imm6_2 >> 1) & 0x3);
  uint32_t imm6 = ((decinfo.isa.instr.c_imm6_2 >> 3) & 0x1);
  uint32_t imm5 = ((decinfo.isa.instr.c_imm6_2) & 0x1);
  uint32_t imm4 = ((decinfo.isa.instr.c_imm6_2 >> 4) & 0x1);
  sword_t simm = (simm9 << 9) | (imm8_7 << 7) | (imm6 << 6) | (imm5 << 5) | (imm4 << 4);
  assert(simm != 0);
  decode_op_i(id_src2, simm, true);
  decode_op_r(id_dest, 2, false);
}

make_DHelper(C_SW) {
  decode_op_r(id_src, creg2reg(decinfo.isa.instr.c_rd_rs1_), true);
  uint32_t imm6 = ((decinfo.isa.instr.c_imm6_5) & 0x1);
  uint32_t imm5_3 = decinfo.isa.instr.c_imm12_10;
  uint32_t imm2 = ((decinfo.isa.instr.c_imm6_5 >> 1) & 0x1);
  word_t imm = (imm6 << 6) | (imm5_3 << 3) | (imm2 << 2);
  decode_op_i(id_src2, imm, true);

  rtl_add(&id_src->addr, &id_src->val, &id_src2->val);

  decode_op_r(id_dest, creg2reg(decinfo.isa.instr.c_rs2_), true);

  decinfo.width = 4;
}

make_DHelper(C_LW) {
  decode_op_r(id_src, creg2reg(decinfo.isa.instr.c_rd_rs1_), true);
  uint32_t imm6 = ((decinfo.isa.instr.c_imm6_5) & 0x1);
  uint32_t imm5_3 = decinfo.isa.instr.c_imm12_10;
  uint32_t imm2 = ((decinfo.isa.instr.c_imm6_5 >> 1) & 0x1);
  word_t imm = (imm6 << 6) | (imm5_3 << 3) | (imm2 << 2);
  decode_op_i(id_src2, imm, true);

  rtl_add(&id_src->addr, &id_src->val, &id_src2->val);

  decode_op_r(id_dest, creg2reg(decinfo.isa.instr.c_rd_), false);

  decinfo.width = 4;
}

make_DHelper(C_SD) {
  decode_op_r(id_src, creg2reg(decinfo.isa.instr.c_rd_rs1_), true);
  uint32_t imm7_6 = decinfo.isa.instr.c_imm6_5;
  uint32_t imm5_3 = decinfo.isa.instr.c_imm12_10;
  word_t imm = (imm7_6 << 6) | (imm5_3 << 3);
  decode_op_i(id_src2, imm, true);

  rtl_add(&id_src->addr, &id_src->val, &id_src2->val);

  decode_op_r(id_dest, creg2reg(decinfo.isa.instr.c_rs2_), true);

  decinfo.width = 8;
}

make_DHelper(C_LD) {
  decode_op_r(id_src, creg2reg(decinfo.isa.instr.c_rd_rs1_), true);
  uint32_t imm7_6 = decinfo.isa.instr.c_imm6_5;
  uint32_t imm5_3 = decinfo.isa.instr.c_imm12_10;
  word_t imm = (imm7_6 << 6) | (imm5_3 << 3);
  decode_op_i(id_src2, imm, true);

  rtl_add(&id_src->addr, &id_src->val, &id_src2->val);

  decode_op_r(id_dest, creg2reg(decinfo.isa.instr.c_rd_), false);

  decinfo.width = 8;
}

make_DHelper(C_J) {
  decode_CJ(pc);
  decode_op_r(id_dest, 0, false);
}

make_DHelper(C_JALR) {
  decode_op_r(id_src, decinfo.isa.instr.c_rd_rs1, true);
  decode_op_r(id_src2, 0, true);
  decode_op_r(id_dest, 1, false);
}

make_DHelper(C_ADDI4SPN) {
  decode_op_r(id_src, 2, true);
  word_t imm = (decinfo.isa.instr.c_imm9_6 << 6) | (decinfo.isa.instr.c_imm5_4 << 4) |
    (decinfo.isa.instr.c_imm3 << 3) | (decinfo.isa.instr.c_imm2 << 2);
  assert(imm != 0);
  decode_op_i(id_src2, imm, true);
  decode_op_r(id_dest, creg2reg(decinfo.isa.instr.c_rd_), false);
}
