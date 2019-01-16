#include "cpu/decode.h"
#include "cpu/rtl.h"

// decode operand helper
#define make_DopHelper(name) void concat(decode_op_, name) (Operand *op, uint32_t val, bool load_val)

static inline make_DopHelper(i) {
  op->type = OP_TYPE_IMM;
  op->imm = val;
  rtl_li(&op->val, op->imm);

  print_Dop(op->str, OP_STR_SIZE, "%d", op->imm);
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
  decode_op_r(id_src, decinfo.arch.instr.rs, true);
  decode_op_i(id_src2, decinfo.arch.instr.simm, true);
  decode_op_r(id_dest, decinfo.arch.instr.rt, false);
}

make_DHelper(IU) {
  decode_op_r(id_src, decinfo.arch.instr.rs, true);
  decode_op_i(id_src2, decinfo.arch.instr.imm, true);
  decode_op_r(id_dest, decinfo.arch.instr.rt, false);

  print_Dop(id_src2->str, OP_STR_SIZE, "0x%x", decinfo.arch.instr.imm);
}

make_DHelper(J) {
  vaddr_t jmp_target = (cpu.pc & 0xf0000000) | (decinfo.arch.instr.jmp_target << 2);
  decode_op_i(id_dest, jmp_target, true);
  decinfo.jmp_pc = jmp_target;

  print_Dop(id_dest->str, OP_STR_SIZE, "0x%x", decinfo.jmp_pc);
}

make_DHelper(R) {
  decode_op_r(id_src, decinfo.arch.instr.rs, true);
  decode_op_r(id_src2, decinfo.arch.instr.rt, true);
  decode_op_r(id_dest, decinfo.arch.instr.rd, false);
}

static inline make_DHelper(addr) {
  decode_op_r(id_src, decinfo.arch.instr.rs, true);
  decode_op_i(id_src2, decinfo.arch.instr.simm, true);

  print_Dop(id_src->str, OP_STR_SIZE, "%d(%s)", id_src2->val, reg_name(id_src->reg, 4));

  rtl_add(&id_src->addr, &id_src->val, &id_src2->val);
}

make_DHelper(ld) {
  decode_addr(eip);
  decode_op_r(id_dest, decinfo.arch.instr.rt, false);
}

make_DHelper(st) {
  decode_addr(eip);
  decode_op_r(id_dest, decinfo.arch.instr.rt, true);
}

make_DHelper(B) {
  int32_t offset = (decinfo.arch.instr.simm << 2);
  decinfo.jmp_pc = cpu.pc + offset + 4;

  decode_op_r(id_src, decinfo.arch.instr.rs, true);
  decode_op_r(id_src2, decinfo.arch.instr.rt, true);
  decode_op_i(id_dest, decinfo.jmp_pc, true);

  print_Dop(id_dest->str, OP_STR_SIZE, "0x%x", decinfo.jmp_pc);
}

make_DHelper(shift) {
  decode_op_i(id_src, decinfo.arch.instr.sa, true);
  decode_op_r(id_src2, decinfo.arch.instr.rt, true);
  decode_op_r(id_dest, decinfo.arch.instr.rd, false);
}

make_DHelper(cmov) {
  decode_op_r(id_src, decinfo.arch.instr.rs, true);
  decode_op_r(id_src2, decinfo.arch.instr.rt, true);
  decode_op_r(id_dest, decinfo.arch.instr.rd, true);
}
